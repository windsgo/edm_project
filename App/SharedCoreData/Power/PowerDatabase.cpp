#include "PowerDatabase.h"
#include "ui_PowerDatabase.h"

#include <QFile>
#include <QHeaderView>
#include <QMessageBox>

#include <SystemSettings/SystemSettings.h>

#include "Exception/exception.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

PowerDatabase::PowerDatabase(QWidget *parent)
    : QWidget(parent), ui(new Ui::PowerDatabase) {
    ui->setupUi(this);

    _init_database();
    _init_button_slots();
}

PowerDatabase::~PowerDatabase() { delete ui; }

bool PowerDatabase::get_eleparam_from_index(
    uint32_t index, power::EleParam_dkd_t &output) const {
    QString cmd = QString{"SELECT * FROM %0 WHERE E_INDEX = %1"}
                      .arg(EleTableName)
                      .arg(index);

    QSqlQuery q;
    q.exec(cmd);
    if (!q.next()) {
        return false;
    }

    _get_eleparam_from_record(q.record(), output);
    return true;
}

bool PowerDatabase::exist_index(uint32_t index) const {
    QString cmd = QString{"SELECT EXISTS(SELECT 1 FROM %0 WHERE E_INDEX = %1);"}
                      .arg(EleTableName)
                      .arg(index);
    QSqlQuery q;
    q.exec(cmd);
    q.next();
    bool ret = (q.value(0).toInt() != 0);
    q.finish();
    return ret;
}

std::optional<power::EleParam_dkd_t>
PowerDatabase::get_one_valid_eleparam() const {
    std::optional<power::EleParam_dkd_t> ret{std::nullopt};

    QString cmd = QString{"SELECT * FROM %0;"}.arg(EleTableName);

    QSqlQuery q;
    q.exec(cmd);
    if (!q.next()) {
        return ret;
    }

    power::EleParam_dkd_t p;
    _get_eleparam_from_record(q.record(), p);
    ret = p;

    return ret;
}

void PowerDatabase::_init_database() {
    database_ = QSqlDatabase::addDatabase("QSQLITE");
    QString database_path =
        EDM_CONFIG_DIR +
        QString::fromStdString(
            edm::SystemSettings::instance().get_power_database_file());
    QFile database_file(database_path);
    if (!database_file.exists()) {
        s_logger->warn("power database file does not exists: {}",
                       database_path.toStdString());
        s_logger->info("creating new empty database file: {}",
                       database_path.toStdString());
        database_file.open(QIODevice::ReadWrite); // create this file
        database_file.close();
    }

    database_.setDatabaseName(database_path);

    qDebug() << database_path;

    if (!database_.isValid()) {
        s_logger->error("power database not valid");
        throw exception{"power database not valid"};
    }

    if (!database_.open()) {
        s_logger->error("power database not opened: {}",
                        database_.lastError().text().toStdString());
        throw exception{"power database not opened"};
    }

    // 检查数据表是否存在, 如果不存在就创建空表
    bool table_exists = _check_table_exist(this->EleTableName);
    if (!table_exists) {
        s_logger->warn("table {} not exist, creating new table...",
                       this->EleTableName.toStdString());

        _create_table_with_default_params();
    }

    // 检查各表格列是否正确, 不正确就删除表并重新创建
    bool table_valid = _check_table_valid();
    if (!table_valid) {
        s_logger->warn("table {} not valid, drop it and creating new table...",
                       this->EleTableName.toStdString());

        _drop_table(this->EleTableName);
        _create_table_with_default_params();
    }

    // 将model绑定到该表
    table_model_ = new QSqlTableModel(this, this->database_);
    table_model_->setTable(this->EleTableName);
    table_model_->setSort(
        0, Qt::SortOrder::AscendingOrder); // 对电参数序号(主键)进行排序
    bool ret = table_model_->select();
    if (!ret) {
        s_logger->error("power table model init: select error, abort");
        throw exception{"power table model init: select error"};
    }
    // 初始化model设定
    table_model_->setEditStrategy(
        QSqlTableModel::EditStrategy::OnManualSubmit); // 手动确认

    // 修改表头文字
    for (int i = 0; i < table_model_->columnCount(); ++i) {
        auto header =
            table_model_->headerData(i, Qt::Orientation::Horizontal).toString();
        if (header.startsWith("E_")) {
            header = header.mid(2);
        }

        table_model_->setHeaderData(i, Qt::Orientation::Horizontal, header);
    }

    // 将ui tableview绑定到模型model
    ui->tableView->setModel(table_model_);
    // 初始化ui编辑设定
    ui->tableView->setEditTriggers(EditTrigger); // 双击编辑
    ui->tableView->setSelectionMode(
        QTableView::SelectionMode::SingleSelection); // 单个目标选择
    ui->tableView->setSelectionBehavior(
        QTableView::SelectionBehavior::SelectItems); // 单个item选择

    for (int i = 0; i < table_model_->columnCount(); ++i) {
        auto header =
            table_model_->headerData(i, Qt::Orientation::Horizontal).toString();
        if (header.size() < 4) {
            ui->tableView->setColumnWidth(i, 40);
        } else {
            ui->tableView->setColumnWidth(i, header.size() * 10);
        }
    }

    // 初始化表格状态为不可编辑
    this->_set_table_editable(false);

    // 刷新显示
    this->_update_tableview();
}

void PowerDatabase::_init_button_slots() {
    connect(ui->pb_hide, &QPushButton::clicked, this,
            [this]() { this->hide(); });

    connect(ui->pb_enable_edit, &QPushButton::clicked, this,
            [this](bool checked) {
                if (checked) {
                    this->_set_table_editable(true);
                } else {
                    // 当作cancel
                    this->_slot_tableedit_cancel();
                }
            });

    connect(ui->pb_add, &QPushButton::clicked, this,
            &PowerDatabase::_slot_tableedit_add);
    connect(ui->pb_delete, &QPushButton::clicked, this,
            &PowerDatabase::_slot_tableedit_delete);
    connect(ui->pb_confirm, &QPushButton::clicked, this,
            &PowerDatabase::_slot_tableedit_confirm);
    connect(ui->pb_cancel, &QPushButton::clicked, this,
            &PowerDatabase::_slot_tableedit_cancel);
    connect(ui->pb_refresh, &QPushButton::clicked, this,
            &PowerDatabase::_slot_table_refresh);

    connect(ui->pb_select_param, &QPushButton::clicked, this,
            &PowerDatabase::_slot_choose_eleparam_index);
}

bool PowerDatabase::_check_table_exist(const QString &table_name) const {
    QString cmd = QString{"SELECT COUNT(*) FROM sqlite_master WHERE "
                          "TYPE='table' and name='%0'"}
                      .arg(table_name);
    QSqlQuery q{this->database_};
    q.exec(cmd);
    q.next();
    bool ret = (q.value(0).toInt() != 0);
    q.finish();
    return ret;
}

bool PowerDatabase::_create_table_with_default_params() {
    const char *create_table_cmd = R"(CREATE TABLE "eleparam" (
                                   "E_INDEX"	INTEGER NOT NULL CHECK("E_INDEX" >= 0 AND "E_INDEX" < 1000) UNIQUE,
                                   "E_ON"	INTEGER NOT NULL DEFAULT 18 CHECK("E_ON" >= 0 AND "E_ON" < 256),
                                   "E_OFF"	INTEGER NOT NULL DEFAULT 48 CHECK("E_OFF" >= 0 AND "E_OFF" < 256),
                                   "E_UP"	INTEGER NOT NULL DEFAULT 5 CHECK("E_UP" >= 0 AND "E_UP" < 256),
                                   "E_DN"	INTEGER NOT NULL DEFAULT 9 CHECK("E_DN" >= 0 AND "E_DN" < 256),
                                   "E_IP"	INTEGER NOT NULL DEFAULT 70 CHECK("E_IP" >= 0 AND "E_IP" < 2560),
                                   "E_HP"	INTEGER NOT NULL DEFAULT 40 CHECK("E_HP" >= 0 AND "E_HP" < 256),
                                   "E_MA"	INTEGER NOT NULL DEFAULT 0 CHECK("E_MA" >= 0 AND "E_MA" < 16),
                                   "E_SV"	INTEGER NOT NULL DEFAULT 5 CHECK("E_SV" >= 0 AND "E_SV" < 256),
                                   "E_AL"	INTEGER NOT NULL DEFAULT 43 CHECK("E_AL" >= 0 AND "E_AL" < 256),
                                   "E_LD"	INTEGER NOT NULL DEFAULT 0 CHECK("E_LD" >= 0 AND "E_LD" < 16),
                                   "E_OC"	INTEGER NOT NULL DEFAULT 0 CHECK("E_OC" >= 0 AND "E_OC" < 16),
                                   "E_PP"	INTEGER NOT NULL DEFAULT 10 CHECK("E_PP" >= 0 AND "E_PP" < 256),
                                   "E_S"	INTEGER NOT NULL DEFAULT 2 CHECK("E_S" >= 0 AND "E_S" < 10),
                                   "E_PL"	INTEGER NOT NULL DEFAULT 0 CHECK("E_PL" >= 0 AND "E_PL" <= 1),
                                   "E_C"	INTEGER NOT NULL DEFAULT 0 CHECK("E_C" >= 0 AND "E_C" < 10),
                                   "E_LV"	INTEGER NOT NULL DEFAULT 1 CHECK("E_LV" >= 0 AND "E_LV" < 2),
                                   "E_JS"	INTEGER NOT NULL DEFAULT 260 CHECK("E_JS" >= 0 AND "E_JS" < 65536),
                                   "E_SV_SENS"	INTEGER NOT NULL DEFAULT 100 CHECK("E_SV_SENS" >= 0 AND "E_SV_SENS" < 256),
                                   "E_SV_VOL_1"	INTEGER NOT NULL DEFAULT 5 CHECK("E_SV_VOL_1" >= 0 AND "E_SV_VOL_1" < 256),
                                   "E_SV_VOL_2"	INTEGER NOT NULL DEFAULT 5 CHECK("E_SV_VOL_2" >= 0 AND "E_SV_VOL_2" < 256),
                                   PRIMARY KEY("E_INDEX" AUTOINCREMENT)
                               );)"; // FROM sqlite browser

    QSqlQuery q;
    bool create_table_ret = q.exec(create_table_cmd);
    if (!create_table_ret) {
        s_logger->critical("create default table failed, {}",
                           q.lastError().text().toStdString());
        throw exception{"create default table failed"};
        return false;
    }

    q.finish();

    // create table finished
    for (int i = 0; i < 10; ++i) {
        q.exec(R"(INSERT INTO "main"."eleparam" DEFAULT VALUES;)");
    }

    // TODO
    return true;
}

bool PowerDatabase::_check_table_valid() const {
    // TODO
    return true;
}

bool PowerDatabase::_drop_table(const QString &table_name) {
    QString cmd = QString{"DROP TABLE %0"}.arg(table_name);
    QSqlQuery q(this->database_);
    bool ret = q.exec(cmd);
    q.finish();
    return ret;
}

void PowerDatabase::_slot_tableedit_confirm() {
    if (!table_model_->submitAll()) {
        s_logger->error("power db error: submit all failed");
        QMessageBox::critical(this, tr("error"),
                              tr("submit failed, revert, error:\n") +
                                  table_model_->lastError().text());
        table_model_->revertAll();
    }

    _set_table_editable(false);
    _update_tableview();

    // 显示出被删除操作暂时隐藏的行
    for (int i = 0; i < table_model_->rowCount(); ++i) {
        ui->tableView->showRow(i);
    }
}

void PowerDatabase::_slot_tableedit_add() {
    auto record = table_model_->record(table_model_->rowCount() - 1);

    //    record.setNull("index");
    table_model_->insertRecord(-1, record);
}

void PowerDatabase::_slot_tableedit_cancel() {
    table_model_->revertAll();

    _set_table_editable(false);
    _update_tableview();

    // 显示出被删除操作暂时隐藏的行
    for (int i = 0; i < table_model_->rowCount(); ++i) {
        ui->tableView->showRow(i);
    }
}

void PowerDatabase::_slot_tableedit_delete() {
    auto row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::critical(this, tr("delete failed"),
                              tr("no row selected, delete failed"));
        return;
    }
    table_model_->removeRow(row);
    ui->tableView->hideRow(row); // 显示隐藏
}

void PowerDatabase::_slot_choose_eleparam_index() {
    auto row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::critical(this, tr("select param failed"),
                              tr("no row selected, select param failed"));
        return;
    }

    // 获取该行的 E_INDEX 值
    auto rec = table_model_->record(row);
    if (rec.isEmpty()) {
        QMessageBox::critical(this, tr("select param failed"),
                              tr("no record, select param failed"));
        return;
    }

    auto e_index_variant = rec.value("E_INDEX");
    if (!e_index_variant.isValid()) {
        QMessageBox::critical(this, tr("select param failed"),
                              tr("no E_INDEX, select param failed"));
        return;
    }

    //    qDebug() << "emit sig_select_param" << e_index_variant.toUInt();
    emit sig_select_param_index(e_index_variant.toUInt());

    power::EleParam_dkd_t param;
    _get_eleparam_from_record(rec, param);
    emit sig_select_param(param);
}

void PowerDatabase::_slot_table_refresh() {
    this->_update_tableview();

    // test

    // qDebug() << this->exist_index(4);
    // qDebug() << this->exist_index(10);

    // qDebug() << (bool)this->get_eleparam_from_index(4);
    // qDebug() << (bool)this->get_eleparam_from_index(10);

    // if (auto ret = this->get_eleparam_from_index(4)) {
    //     auto ele = *ret;
    // }
}

void PowerDatabase::_set_table_editable(bool editable) {
    // TODO Others

    // buttons disabled when editting
    ui->pb_enable_edit->setChecked(editable);
    //    ui->pb_enable_edit->setEnabled(!editable);
    ui->pb_refresh->setEnabled(!editable);
    ui->pb_hide->setEnabled(!editable);
    ui->pb_select_param->setEnabled(!editable);

    // buttons enabled when editting
    ui->pb_add->setEnabled(editable);
    ui->pb_delete->setEnabled(editable);
    ui->pb_confirm->setEnabled(editable);
    ui->pb_cancel->setEnabled(editable);

    if (editable) {
        ui->tableView->setEditTriggers(EditTrigger);
    } else {
        ui->tableView->setEditTriggers(QTableView::EditTrigger::NoEditTriggers);
    }
}

void PowerDatabase::_update_tableview() { table_model_->select(); }

void PowerDatabase::_get_eleparam_from_record(const QSqlRecord &record,
                                              power::EleParam_dkd_t &output) {

#define XX_(ele_name__, db_name__) \
    output.ele_name__ = record.value(#db_name__).toUInt();

    XX_(pulse_on, E_ON)
    XX_(pulse_off, E_OFF)
    XX_(up, E_UP)
    XX_(dn, E_DN)
    XX_(ip, E_IP)
    XX_(hp, E_HP)
    XX_(ma, E_MA)
    XX_(sv, E_SV)
    XX_(al, E_AL)
    XX_(ld, E_LD)
    XX_(oc, E_OC)
    XX_(pp, E_PP)
    XX_(servo_speed, E_S)
    XX_(pl, E_PL) // 极性 0 为 正 ; 1 为 负
    XX_(c, E_C)
    XX_(lv, E_LV)
    XX_(jump_js, E_JS)
    XX_(servo_sensitivity, E_SV_SENS)
    XX_(UpperThreshold, E_SV_VOL_1)
    XX_(LowerThreshold, E_SV_VOL_2)

    XX_(upper_index, E_INDEX)

#undef XX_
}

} // namespace app
} // namespace edm
