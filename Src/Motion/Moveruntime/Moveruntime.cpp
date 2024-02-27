#include "Moveruntime.h"
#include <cmath>

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER_NAME(s_logger, "motion");

static double _get_target_length_iod(double vi, double vo, double d1, double d2,
                                     double T) {
    return ((vi + vo) * (d1 + 0.5 * d2) + (vo - vi) * T);
}

static double jerk2acc(int32_t N, double T) { return N * T; }

namespace edm {

namespace move {

Moveruntime::Moveruntime() noexcept
    : target_length_(0.0), current_length_(0.0), state_(State::NotRunning) {
    impl_.clear();
}

bool Moveruntime::plan(const MoveRuntimePlanSpeedInput& speed_param,
                       unit_t target_length) {
    if (is_running()) {
        s_logger->trace("moveruntime plan warn: still running");
        //! 不报错, 因为中断减速一般需要replan, 直接调用plan可以节省一次外部clear的开销
    }

    // 长度为0则不运动
    if (target_length <= 0.0) {
        s_logger->error("moveruntime plan error: length not valid: {}", target_length);
        return false;
    }

    this->clear();

    int32_t ret = 0;

    uint32_t Nacc = speed_param.nacc; // Nacc值

    /* 设置参数值 */
    // 设置Nacc
    if (Nacc < 1) {
        Nacc = 1;
    } else if (Nacc > 200) {
        Nacc = 200;
    }

    // 设置加速度, 速度阈值
    impl_.acc0 = speed_param.acc0;
    impl_.dec0 = speed_param.dec0;
    impl_.cruise_velocity = speed_param.cruise_v;
    impl_.entry_velocity = speed_param.entry_v;
    impl_.exit_velocity = speed_param.exit_v;
    impl_.length = target_length;

    this->target_length_ = target_length;

    /* 速度规划 */

    static const double T = EDM_SERVO_PEROID_US / 1000000.0;
    static const double TCube = T * T * T;
    double v0, s0, j1, s1, s2, j3, s3, v4, s4, j5, s5, s6, j7, s7, s123, s567,
        si2o;
    double d0, d1, d2, d5, d6;
    int32_t N1, N2, N3, N4, N5, N6, N7;

    double vc = impl_.cruise_velocity;
    double vi = impl_.entry_velocity;
    double vo = impl_.exit_velocity;

    if (vo <= 0) {
        vo = 2.0;
    }

    //    vi = 0.1*vc;
    //    vo = 0.2*vc;
    //    impl_.entry_velocity = vi;
    //    impl_.exit_velocity = vo;

    double s = impl_.length;
    double ai, ao, at;

    d0 = jerk2acc(Nacc, T);
    impl_.T = T;

    if (fabs(vi - vo) < 0.00001) {
        at = 0;
    } else if (vi < vo) {
        at = impl_.acc0;
    } else {
        at = impl_.dec0;
    }

    //    get si2o
    if (fabs(at) > 0) {
        j1 = at / d0;

        N2 = floor((vo - vi - at * d0) / at / T); // acc /dec
        if (N2 < 1) {
            N2 = 0;
            N1 = floor(sqrt((vo - vi) / j1) / T); //>=1
            if (N1 < 2) {
                N1 = 2;
            }
            N3 = N1;
            d1 = jerk2acc(N1, T);
            j1 = (vo - vi) / d1 / d1;
            j3 = -1.0 * j1;
            si2o = _get_target_length_iod(vi, vo, d1, 0, T);
        } else {
            N1 = N3 = Nacc;
            d1 = d0;
            d2 = jerk2acc(N2, T);
            j1 = (vo - vi) / d1 / (d1 + d2);
            j3 = -1.0 * j1;
            si2o = _get_target_length_iod(vi, vo, d1, d2, T);
        }
    } else {
        si2o = 0;
    }

    if (s <= si2o) {
        /*if(s < 0.000005){
            j1 = s/TCube;
            N1 = 1;
            j5=j7 = 0;
            N2=N3=N4=N5=N6=N7 = 0;
            s123 = s;
            ret = 1;
        }
        else*/
        //        {
        N2 = floor((s - (vi + vo) * d0 - (vo - vi) * T) / (vo + vi) / 0.5 / T);
        if (N2 < 1) {
            N2 = 0;
            d1 = (s - (vo - vi) * T) / (vo + vi);
            N1 = floor(d1 / T);
            if (N1 < 2) {
                N1 = 2;
            }
            d1 = jerk2acc(N1, T);
            j1 = (vo - vi) / d1 / d1;
            j3 = -1.0 * j1;
            N3 = N1;
            s123 = _get_target_length_iod(vi, vo, d1, 0, T);
        } else {
            N1 = N3 = Nacc;
            d1 = d0;
            d2 = jerk2acc(N2, T);
            j1 = (vo - vi) / d1 / (d1 + d2);
            j3 = -1.0 * j1;
            s123 = _get_target_length_iod(vi, vo, d1, d2, T);
        }

        //        }
        N4 = 0;
        s4 = 0;
        j5 = j7 = 0;
        N5 = N6 = N7 = 0;
        s567 = 0;
    } else { /*s > si2o*/
        if (fabs(vi - vc) < 0.00001) {
            N1 = N2 = N3 = 0;
            s1 = s2 = s3 = 0;
            s123 = 0;
            ai = 0;
        } /*vi = vc */
        else if (vi < vc) {
            ai = impl_.acc0;
        } else {
            ai = impl_.dec0;
        }
        j1 = ai / d0;

        if (fabs(ai) > 0) {                           /*v-->s*/
            N2 = floor((vc - vi - ai * d0) / ai / T); // acc /dec
            if (N2 < 1) {
                N2 = 0;
                N1 = floor(sqrt((vc - vi) / j1) / T); //>=1
                if (N1 < 2) {
                    N1 = 2;
                }
                N3 = N1;
                d1 = jerk2acc(N1, T);
                //        d11 = jerk2vel(N1,T);
                j1 = (vc - vi) / d1 / d1;
                j3 = -1.0 * j1;
                s123 = _get_target_length_iod(vi, vc, d1, 0, T);
                //        s123 = 2*vi*d1 + 2*j1*d1*d11;
            } else {
                N1 = N3 = Nacc;
                d1 = d0;
                d2 = jerk2acc(N2, T);
                //        d11 = jerk2vel(N1,T);
                //        d22 = jerk2vel(N2,T);
                j1 = (vc - vi) / d1 / (d1 + d2);
                j3 = -1.0 * j1;
                s123 = _get_target_length_iod(vi, vc, d1, d2, T);
                //        s123 = vi*(2*d1+d2) +2*j1*d1*d11 + j1*(d11+d1*d1)*d2 +
                //        j1*d1*d22;
            }
        }

        if (fabs(vo - vc) < 0.00001) {
            N5 = N6 = N7 = 0;
            s5 = s6 = s7 = 0;
            s567 = 0;
            ao = 0;
        } else if (vo < vc) {
            ao = impl_.dec0;
        } else {
            ao = impl_.acc0;
        }

        j5 = ao / Nacc / T;

        if (fabs(ao) > 0) {                           /*v-->s*/
            N6 = floor((vo - vc - ao * d0) / ao / T); // dec
            if (N6 < 1) {
                N6 = 0;
                N5 = floor(sqrt((vo - vc) / j5) / T); //>=1
                if (N5 < 2) {
                    N5 = 2;
                }
                N7 = N5;
                d5 = jerk2acc(N5, T);
                j5 = (vo - vc) / d5 / d5;
                j7 = -1.0 * j5;
                s567 = _get_target_length_iod(vc, vo, d5, 0, T);
            } else {
                N5 = N7 = Nacc;
                d5 = d0;
                d6 = jerk2acc(N6, T);

                j5 = (vo - vc) / d5 / (d5 + d6);
                j7 = -1.0 * j5;
                s567 = _get_target_length_iod(vc, vo, d5, d6, T);
            }
        }

        s4 = s - s123 - s567;
        v4 = vc;
        N4 = floor(s4 / v4 / T);
        s4 = v4 * N4 * T;

        if (s4 < 0) {
            if (fabs(ai) < 0.00001 && fabs(ao) > 0.00001) {
                ai = ao;
            } else if (fabs(ao) < 0.00001 && fabs(ai) > 0.00001) {
                ao = ai;
            }

            if ((ai < 0 && ao < 0) || (ai > 0 && ao > 0)) {
                N2 = floor((s - (vi + vo) * d0 - (vo - vi) * T) / (vo + vi) /
                           0.5 / T);
                if (N2 < 1) {
                    N2 = 0;
                    d1 = (s - (vo - vi) * T) / (vo + vi);
                    N1 = floor(d1 / T);
                    if (N1 < 2) {
                        N1 = 2;
                        ret = 2;
                    }
                    d1 = jerk2acc(N1, T);
                    j1 = (vo - vi) / d1 / d1;
                    j3 = -1.0 * j1;
                    N3 = N1;
                    s123 = _get_target_length_iod(vi, vo, d1, 0, T);
                } else {
                    N1 = N3 = Nacc;
                    d1 = d0;
                    d2 = jerk2acc(N2, T);
                    j1 = (vo - vi) / d1 / (d1 + d2);
                    j3 = -1.0 * j1;
                    s123 = _get_target_length_iod(vi, vo, d1, d2, T);
                }

                N4 = 0;
                j5 = j7 = 0;
                N5 = N6 = N7 = 0;
                s567 = 0;
            }

            if ((ai > 0 && ao < 0) || (ai < 0 && ao > 0)) {
                // todo reduce ai  version 1.0.0 Tshape to instead Sshape
                //             ai = 0.5*ai;
                //             double vcf2 = ai*s + 0.5*(vi*vi + vo*vo);
                //             /*approxmate VCF*/ s123 = (vcf2 - vi*vi)/(2*ai);
                //             /* # ?????????????*/ vc = sqrt(vcf2);

                /*version 2.0.0*/
                if (at * ai > 0) {
                    s123 = 0.5 * (s + si2o);
                } else if (at * ai < 0) {
                    s123 = 0.5 * (s - si2o);
                } else {
                    s123 = 0.5 * s;
                }

                double xa = 0.5 * ai;
                double xb = vi + 1.5 * ai * d0 + ai * T;
                double xc = d0 * (2 * vi + ai * d0 + ai * T) - s123;

                d2 = (-xb + sqrt(xb * xb - 4 * xa * xc)) / (2 * xa);

                //            N2 = floor((s123 - (vi+vc)*d0 - (vc-vi)*T) /
                //            (0.5*(vc+vi)*T));//acc /dec
                j1 = ai / d0;
                if (d2 < 0) {
                    N2 = 0;

                    double xp = 2 * vi / j1;
                    double xq = -1.0 * s123 / j1;

                    if (j1 > 0) {
                        double xt = sqrt(xq * xq / 4 + xp * xp * xp / 27);
                        d1 = cbrt(-0.5 * xq + xt) + cbrt(-0.5 * xq - xt);
                    } else {
                        double r0 = sqrt(xp / (-3.0));
                        double r = r0 * xp / (-3.0);
                        double theta = acos(-0.5 * xq / r);
                        d1 = 2.0 * r0 * cos((theta + 3.1415926 * 4) / 3);
                        //                        d1 = 2.0*r0*cos(theta /3);
                    }

                    vc = vi + j1 * d1 * d1; /*v2.0*/
                    N1 = floor(d1 / T);

                    if (N1 < 2) {
                        N1 = 2;
                    }

                    d1 = jerk2acc(N1, T);
                    j1 = (vc - vi) / d1 / d1;
                    N3 = N1;
                    j3 = -1.0 * j1;
                    s123 = _get_target_length_iod(vi, vc, d1, 0, T);
                } else {

                    N1 = N3 = Nacc;
                    d1 = jerk2acc(N1, T);

                    vc = vi + ai * (d1 + d2); /*v2.0*/

                    N2 = floor(d2 / T);
                    d2 = jerk2acc(N2, T);
                    j1 = (vc - vi) / d1 / (d1 + d2);
                    j3 = -1.0 * j1;

                    s123 = _get_target_length_iod(vi, vc, d1, d2, T);
                }

                s567 = s - s123;
                N6 = floor((s567 - (vo + vc) * d0 - (vo - vc) * T) /
                           (0.5 * (vc + vo) * T)); ///*v--> s*/
                if (N6 < 1) {
                    N6 = 0;

                    d5 = (s567 - (vo - vc) * T) / (vc + vo);
                    N5 = floor(d5 / T);
                    if (N5 < 2) {
                        N5 = 2;
                    }
                    d5 = jerk2acc(N5, T);
                    j5 = (vo - vc) / d5 / d5;
                    N7 = N5;
                    j7 = -1.0 * j5;
                    s567 = _get_target_length_iod(vc, vo, d5, 0, T);
                } else {
                    N5 = N7 = Nacc;
                    d5 = d0;
                    d6 = jerk2acc(N6, T);
                    j5 = (vo - vc) / d5 / (d5 + d6);
                    j7 = -1.0 * j5;
                    s567 = _get_target_length_iod(vc, vo, d5, d6, T);
                }
            }

#if 0
            if(0){
//todo reduce ai  version 1.0.0 Tshape to instead Sshape
            ai = 0.5*ai;
//            double vcf2 = ai*s + 0.5*(vi*vi + vo*vo);   /*approxmate VCF*/
//            s123 = (vcf2 - vi*vi)/(2*ai);  /* # ?????????????*/
//            vc = sqrt(vcf2);

                /*version 2.0.0*/
                if(at < 0){
                    s123 = 0.5*(s + si2o);
                }
                else if(at > 0){
                    s123 = 0.5*(s - si2o);
                }
                else {
                    s123 = 0.5*s;
                }


                vc = sqrt(ai*s + 0.5*(vi*vi + vo*vo));

                N2 = floor((s123 - (vi+vc)*d0 - (vc-vi)*T) / (vc+vi)/0.5/T);
                if(N2 < 1){
                    N2 = 0;
                    d1 = (s123 - (vc-vi)*T) / (vc+vi);
                    N1 = floor(d1/T);
                    if(N1<2){
                        N1 = 2;
                        ret = 2;
                    }
                    d1 = jerk2acc(N1,T);
                    j1 = (vc-vi) / d1/d1;
                    j3 = -1.0*j1;
                    N3 = N1;
                    s123 = _get_target_length_iod(vi,vc,d1,0,T);
                }
                else {
                    N1=N3 = Nacc;
                    d1 = d0;
                    d2 = jerk2acc(N2,T);
                    j1 = (vc-vi) / d1/(d1+d2);
                    j3 = -1.0*j1;
                    s123 =_get_target_length_iod(vi,vc,d1,d2,T);
                }


                s567 = s - s123;
                N6 = floor((s567 - (vo+vc)*d0 - (vo-vc)*T) / (0.5*(vc+vo)*T));///*v--> s*/
                if(N6<1){
                    N6 = 0;

                    d5 = (s567 - (vo-vc)*T)/(vc+vo);
                    N5 = floor(d5/T);
                    if(N5<2){
                        N5 = 2;
                    }
                    d5 = jerk2acc(N5,T);
                    j5 = (vo-vc)/d5/d5;
                    N7 = N5;
                    j7 = -1.0*j5;
                    s567 = _get_target_length_iod(vc,vo,d5,0,T);
                }
                else {
                    N5=N7 = Nacc;
                    d5 = d0;
                    d6 = jerk2acc(N6,T);
                    j5 = (vo-vc) / d5/(d5+d6);
                    j7 = -1.0*j5;
                    s567 = _get_target_length_iod(vc,vo,d5,d6,T);
                }

            }
#endif // 0
            N4 = 0;
            s4 = 0;
            //            s0 = s - s123-s4-s567;

            //            if((N1>0)&&(N5>0)){
            //                v0 = 2*s0/(N4 + N1+N2+N3+N4+N5+N6+N7 -2)/T;//
            //                #???????? impl_.a13 = v0/((N1+N2+N3-1)*T);
            //                impl_.a57 = -1*v0/((N5+N6+N7-1)*T);
            //                impl_.intermediate_e = 0;
            //            }
            //            else if(N1>0){
            //                v0 = 2*s0/(N4 + N1+N2+N3+N4 - 2)/T;// #??????
            //                impl_.a13 = v0/((N1+N2+N3-1)*T);
            //                impl_.a57 = 0;
            //                impl_.intermediate_e = 0;
            //            }
            //            else if(N5>0){
            //                v0 = 2*s0/(N4 +N4+N5+N6+N7 - 2)/T;// #?????
            //                impl_.a13 = 0;
            //                impl_.a57 = v0/((N5+N6+N7-1)*T);
            //                impl_.intermediate_e = 0;
            //            }
            //            else {
            //                v0 = s0/(N4-1)/T;// #?????
            //                impl_.a13 = 0;
            //                impl_.a57 = 0;
            //                impl_.intermediate_e = v0;
            //            }
        }
    }

    s0 = s - s123 - s4 - s567;

    if (1) {
        if ((N1 > 0) && (N5 > 0)) {
            v0 = 2 * s0 / (N4 + N1 + N2 + N3 + N4 + N5 + N6 + N7 - 2) /
                 T; // #????????
            impl_.a13 = v0 / ((N1 + N2 + N3 - 1) * T);
            impl_.a57 = -1 * v0 / ((N5 + N6 + N7 - 1) * T);
            impl_.intermediate_e = 0;
        } else if (N1 > 0) {
            v0 = s0 / (N1 + N2 + N3 + N4 - 1) / T; // #??????
            impl_.a13 = 0;
            impl_.a57 = 0;
            impl_.intermediate_e = v0;
        } else if (N5 > 0) {
            v0 = s0 / (N4 + N5 + N6 + N7 - 1) / T; // #?????
            impl_.a13 = 0;
            impl_.a57 = 0; // v0/((N5+N6+N7-1)*T);
            impl_.intermediate_e = v0;
        } else {
            v0 = s0 / (N4 - 1) / T; // #?????
            impl_.a13 = 0;
            impl_.a57 = 0;
            impl_.intermediate_e = v0;
        }
    }

    impl_.N1 = N1;
    impl_.N2 = N2;
    impl_.N3 = N3;
    impl_.N4 = N4;
    impl_.N5 = N5;
    impl_.N6 = N6;
    impl_.N7 = N7;
    impl_.j1 = j1 * TCube;
    //    impl_.j2 = j2 *T*T*T;
    impl_.j3 = j3 * TCube;
    impl_.j5 = j5 * TCube;
    //    impl_.j6 = j6 *T*T*T;
    impl_.j7 = j7 * TCube;
    impl_.a13 = impl_.a13 * T * T;
    impl_.a57 = impl_.a57 * T * T;
    impl_.total_N = N1 + N2 + N3 + N4 + N5 + N6 + N7;

    impl_.current_N = 0;

    impl_.entry_acc = 0;
    impl_.exit_acc = 0;

    //    impl_.entry_jerk = impl_.entry_jerk *T*T*T;
    //    impl_.exit_jerk = impl_.exit_jerk *T*T*T;

    impl_.intermediate_p = vi * T;
    impl_.intermediate_e = impl_.intermediate_e * T;
    impl_.recordintermediate_p = 0;
    impl_.recordintermediate_e = 0;

    this->state_ = State::Running;
    return true;
}

void Moveruntime::clear() {
    target_length_ = 0.0;
    current_length_ = 0.0;
    state_ = State::NotRunning;

    impl_.clear();
}

unit_t Moveruntime::run_once() {
    if (is_over()) {
        return 0;
    }

    if (impl_.current_N < impl_.total_N) {

        if (impl_.current_N < impl_.N1) {
            impl_.entry_acc += impl_.j1;
            impl_.intermediate_p += impl_.entry_acc;
        } else if (impl_.current_N < impl_.N1 + impl_.N2) {

            impl_.intermediate_p += impl_.entry_acc;
        } else if (impl_.current_N <
                   impl_.N1 + impl_.N2 + impl_.N3) {
            impl_.entry_acc += impl_.j3;
            impl_.intermediate_p += impl_.entry_acc;
        } else if (impl_.current_N < impl_.N1 + impl_.N2 +
                                                impl_.N3 +
                                                impl_.N4) {
            impl_.intermediate_p += 0;
        } else if (impl_.current_N <
                   impl_.N1 + impl_.N2 + impl_.N3 +
                       impl_.N4 + impl_.N5) {
            impl_.exit_acc += impl_.j5;
            impl_.intermediate_p += impl_.exit_acc;
        } else if (impl_.current_N <
                   impl_.N1 + impl_.N2 + impl_.N3 +
                       impl_.N4 + impl_.N5 + impl_.N6) {
            impl_.intermediate_p += impl_.exit_acc;
        } else {
            impl_.exit_acc += impl_.j7;
            impl_.intermediate_p += impl_.exit_acc;
        }

        if (impl_.current_N <
            impl_.N1 + impl_.N2 + impl_.N3) {
            if (impl_.current_N > 0) {
                impl_.intermediate_e += impl_.a13;
            } else {
                impl_.intermediate_e += 0;
            }
        } else if (impl_.current_N < impl_.N1 + impl_.N2 +
                                                impl_.N3 +
                                                impl_.N4) {
            impl_.intermediate_e += 0;

        } else if (impl_.current_N < impl_.total_N - 1) {
            impl_.intermediate_e += impl_.a57;
        } else {
            impl_.intermediate_e = 0;
        }

        if (impl_.current_N == impl_.total_N - 1) {
            impl_.intermediate_e = 0;
        }

        impl_.intermediate =
            impl_.intermediate_e + impl_.intermediate_p;
        impl_.recordintermediate_e += impl_.intermediate_e;
        impl_.recordintermediate_p += impl_.intermediate_p;
        impl_.recordintermediate = impl_.recordintermediate_e +
                                          impl_.recordintermediate_p;

        impl_.current_N++;

        // this->state_ = State::Running;

        if (impl_.current_N == impl_.total_N) {
            impl_.intermediate -=
                impl_.recordintermediate - impl_.length;
            //            if(impl_.intermediate < 0){
            //                impl_.intermediate += 0.0000001;
            //            }
        }

        impl_.segment_velocity_prev = impl_.segment_velocity;
        impl_.segment_velocity =
            impl_.intermediate / impl_.T;
        
    } else {
        this->state_ = State::NotRunning;
        impl_.intermediate = 0;
    }

    //! 这个值不一定严格等于 total_length_, 会有累积舍入误差
    this->current_length_ += impl_.intermediate;

    // 返回增量
    return impl_.intermediate;

    // if (p_increments != NULL) {
    //     for (int i = 0; i < MOVE_AXES_NUM; ++i) {
    //         p_increments->value[i] = impl_.increment[i];
    //     }
    // }

    // There are 3 things that can happen here depending on return conditions:
    //	  status	 impl_.move_state	 Description
    //    ---------	 -------------- ----------------------------------------
    //	  STAT_EAGAIN	 <don't care>	 MoveRuntime buffer has more segments to
    //run 	  STAT_OK		 MOVE_STATE_RUN	 MoveRuntime and moveruntime
    //buffers are done 	  STAT_OK		 MOVE_STATE_NEW	 MoveRuntime done;
    //moveruntime must be run again (it's been reused)

    // if (status != STAT_EAGAIN) {
    //     impl_.move_state = MOVE_STATE_OFF;			// the
    //     current line is over and reset MoveRuntime buffer
    //     impl_.section_state = MOVE_STATE_OFF;
    //     impl_.Motion_Type = POINT_MOVE_NOMOVE;
    // }
}

unit_t Moveruntime::get_current_speed() const {
    if (state_ != State::Running) {
        return 0.0; // 不在运行中, 返回速度0
    }

    return impl_.segment_velocity; // 速度值
}

} // namespace move

} // namespace edm
