from __future__ import annotations
from enum import Enum, unique
import json
from inspect import stack

__all__ = ["InterpreterException", "RS274Interpreter", "Points"]
        

@unique
class CoordinateMode(Enum):
    Undefined = -1
    AbsoluteMode = 0 # G90
    IncrementMode = 1 # G91

@unique
class MotionMode(Enum):
    Undefined = -1
    G00 = 0
    G01 = 1
    DRILL = 2

# 指令类型枚举
@unique
class CommandType(Enum):
    Undefined = -1
    G00MotionCommand = 0 # g00
    G01MotionCommand = 1 # g01
    CoordinateModeCommand = 2 # g90 g91
    CoordinateIndexCommand = 3 # g54, g55, etc.
    EleparamSetCommand = 4 # e001
    FeedSpeedSetCommand = 5 # f1000
    DelayCommand = 6 # g04t5
    CoordSetZeroCommand = 7 # coord_set_x_zero, coord_set_zero(x=True, z=True), coord_set_all_zero(), etc
    DrillMotionCommand = 8 # 打孔指令
    G01GroupMotionCommand = 9 # g01_group
    PauseCommand = 98 # m00
    ProgramEndCommand = 99 # m02

# 输出的命令字典(json)中的可用key
@unique
class CommandDictKey(Enum): 
    Undefined = -1
    CommandType = 0
    CoordinateMode = 1
    CoordinateIndex = 2
    MotionMode = 3
    Coordinates = 4
    LineNumber = 5
    EleparamIndex = 6
    FeedSpeed = 7
    DelayTime = 8
    M05IgnoreTouchDetect = 9
    G00Touch = 10
    SetZeroAxisList = 11
    DrillDepth = 12
    DrillHoldTime = 13
    DrillTouch = 14
    DrillBreakout = 15
    DrillBack = 16
    DrillSpindleSpeed = 17
    G01GroupPoints = 18
    CommandStr = 19


class InterpreterException(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)
        if (len(args) > 0 and isinstance(args[0], str)):
            self.error_info = args[0]
        else:
            self.error_info = 'Unknown Error'
            
        # self.error_info = _add_prefix_to_each_line_of_str(self.error_info, "**   ")
    def __str__(self) -> str:
        return self.error_info

# 点列, 用于一组点, 可以用于进行一系列连续的运动(如G01连续, 便于跨段回退; 或表示Nurbs曲线点)
class Points(object):
    class _Point(object):
        def __init__(self, coordinate_mode : CoordinateMode, line_number, code_str) -> None:
            self.__coordinate_mode : CoordinateMode = coordinate_mode
            self.__coords : list = [None, None, None, None, None, None]
            self.__line_number : int = line_number
            self.__code_str : str = code_str
        
        def set_point(self, x: float = None, y: float = None, z: float = None, b: float = None, c: float = None, a: float = None) -> None:
            self.__coords = [x, y, z, b, c, a]
            for value in self.__coords:
                if ((value is not None) and (not isinstance(value, (int, float)))):
                    raise InterpreterException(f"Point value type invalid: {type(value)} ")
        
        def get_point(self) -> list:
            return self.__coords

        def get_coordinate_mode(self) -> CoordinateMode:
            return self.__coordinate_mode
        
        def get_line_number(self) -> int:
            return self.__line_number
        
        def get_code_str(self) -> str:
            return self.__code_str
    
    def __init__(self) -> None:
        self.__points : list[Points._Point] = []
        self.__current_coordinate_mode = CoordinateMode.Undefined
        
    def _check_environment(self) -> None:
        if (self.__current_coordinate_mode == CoordinateMode.Undefined):
            raise InterpreterException("CoordinateMode Undefined " + _get_stackmessage(2))
    
    def g90(self) -> Points:
        self.__current_coordinate_mode = CoordinateMode.AbsoluteMode
        return self

    def g91(self) -> Points:
        self.__current_coordinate_mode = CoordinateMode.IncrementMode
        return self
    
    def push(self, x: float = None, y: float = None, z: float = None, b: float = None, c: float = None, a: float = None) -> Points:
        self._check_environment()
        point = self._Point(self.__current_coordinate_mode, _get_caller_linenumber(), _get_caller_code())
        try:
            point.set_point(x, y, z, b, c, a)
        except Exception as e:
            raise InterpreterException(e.__str__() + _get_stackmessage())
        
        self.__points.append(point)
        
        return self
    
    def get_points(self) -> list[Points._Point]:
        return self.__points
    
    def test_print(self) -> Points:
        for p in self.__points:
            print(f"mode: {p.get_coordinate_mode().name}, point: {p.get_point()}, line: {p.get_line_number()}")
        return self

def _add_prefix_to_each_line_of_str(input: str, prefix: str) -> str:
    output = ""
    for line in input.splitlines():
        output += prefix
        output += line
        output += "\n"
    
    return output

class Environment(object):
    def __init__(self) -> None:
        self.__coordinate_mode = CoordinateMode.Undefined # 坐标绝对、增量模式
        self.__motion_mode = MotionMode.Undefined # 运动模式G00 G01等
        self.__coordinate_index = -1 # 当前使用的坐标系序号
        self.__program_end_flag = False # 程序结束标志, 有此标志后应当不再解释后续指令
        self.__feed_speed = -1 # 当前设定的进给率
        
    def set_coordinate_mode(self, mode : CoordinateMode) -> None:
        assert(isinstance(mode, CoordinateMode))
        self.__coordinate_mode = mode
    def get_coordinate_mode(self) -> CoordinateMode:
        return self.__coordinate_mode
    
    def set_motion_mode(self, mode : MotionMode) -> None:
        assert(isinstance(mode, MotionMode))
        self.__motion_mode = mode
    def get_motion_mode(self) -> MotionMode:
        return self.__motion_mode
    
    def set_coordinate_index(self, index : int) -> None:
        assert(isinstance(index, (int, float)))
        self.__coordinate_index = index
    def get_coordinate_index(self) -> int:
        return self.__coordinate_index
    
    def set_program_end_flag(self) -> None:
        self.__program_end_flag = True
    def is_program_end(self) -> bool:
        return self.__program_end_flag
    
    def set_feed_speed(self, feed_speed : int) -> None:
        assert(isinstance(feed_speed, int))
        self.__feed_speed = feed_speed
    def get_feed_speed(self) -> int:
        return self.__feed_speed
    
def _get_caller_linenumber(layer = 1) -> int:
    return stack()[1 + layer].lineno

def _get_caller_code(layer = 1) -> str:
    return stack()[1 + layer].code_context[0][:-1]

def _get_stackmessage(layer = 1) -> str:
    # call_linenum = currentframe().f_back.f_back.f_lineno
    call_linenum = _get_caller_linenumber(layer + 1)
    call_code = _get_caller_code(layer + 1)
    # stack_message = f"In Line {call_linenum}:\n{call_code}"
    stack_message = f"(line {call_linenum}): {call_code}"
    
    return stack_message

class RS274Interpreter(object):
    def __init__(self) -> None:
        self.__g_environment = Environment() # 初始化解释器全局环境
        self.__g_command_list = [] # 解释出的command列表
        
    # 检查环境是否valid, 在输出运动指令时调用, 以防止未定义的模式
    def _assert_environment_valid(self) -> None:
        if (self.__g_environment.get_coordinate_mode() == CoordinateMode.Undefined):
            raise InterpreterException("CoordinateMode Undefined " + _get_stackmessage(2))

        if (self.__g_environment.get_motion_mode() == MotionMode.Undefined):
            raise InterpreterException("MotionMode Undefined " + _get_stackmessage(2))
        
        if (self.__g_environment.get_coordinate_index() == -1):
            raise InterpreterException("Coordinate Not Set " + _get_stackmessage(2))
        elif (self.__g_environment.get_coordinate_index() < 0): # TODO 坐标系index检测合法
            raise InterpreterException(f"Coordinate Index ({self.__g_environment.get_coordinate_index()}) not valid " + _get_stackmessage(2))
    
        if (self.__g_environment.get_feed_speed() < 0):
            raise InterpreterException("Feed Speed Not Set" + _get_stackmessage(2))
    
    # 设置绝对坐标模式
    def g90(self) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        self.__g_environment.set_coordinate_mode(CoordinateMode.AbsoluteMode)
        command = {
            CommandDictKey.CommandType.name: CommandType.CoordinateModeCommand.name,
            CommandDictKey.CoordinateMode.name: CoordinateMode.AbsoluteMode.name,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self
    
    # 设置相对坐标模式
    def g91(self) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        self.__g_environment.set_coordinate_mode(CoordinateMode.IncrementMode)
        command = {
            CommandDictKey.CommandType.name: CommandType.CoordinateModeCommand.name,
            CommandDictKey.CoordinateMode.name: CoordinateMode.IncrementMode.name,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self

    # 延时
    def g04(self, t : float) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(t, (int, float))):
            raise InterpreterException(f"Delay Time type invalid: {type(t)} " + _get_stackmessage())
        
        if (t < 0):
            raise InterpreterException(f"Delay Time ({t}) Out of Range " + _get_stackmessage())
        
        command = {
            CommandDictKey.CommandType.name: CommandType.DelayCommand.name,
            CommandDictKey.DelayTime.name: t,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self
    
    # 设置电参数号
    def e(self, index : int) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(index, int)):
            raise InterpreterException(f"Eleparam Index type invalid: {type(index)} " + _get_stackmessage())
        
        if (index < 0 or index >= 1000):
            raise InterpreterException(f"Eleparam Index ({index}) Out of Range " + _get_stackmessage())
        
        command = {
            CommandDictKey.CommandType.name: CommandType.EleparamSetCommand.name,
            CommandDictKey.EleparamIndex.name: index,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self
    
    # 设置坐标轴序号指令
    def _command_set_coordinate_index(self, index : int) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(index, int)):
            raise InterpreterException(f"Coordinate Index Type invalid: {type(index)} " + _get_stackmessage(2))
        
        if (index < 0):
            raise InterpreterException(f"Coordinate Index ({index}) < 0 " + _get_stackmessage(2))
        
        self.__g_environment.set_coordinate_index(index)
        command = {
            CommandDictKey.CommandType.name: CommandType.CoordinateIndexCommand.name,
            CommandDictKey.CoordinateIndex.name: index,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(2), # indirect call
            CommandDictKey.CommandStr.name: _get_caller_code(2)
        }
        
        self.__g_command_list.append(command)
        return self
    
    def g53(self) -> RS274Interpreter:
        return self._command_set_coordinate_index(53)
    def g54(self) -> RS274Interpreter:
        return self._command_set_coordinate_index(54)
    def g55(self) -> RS274Interpreter:
        return self._command_set_coordinate_index(55)
    def g56(self) -> RS274Interpreter:
        return self._command_set_coordinate_index(56)
    def set_coord(self, index: int) -> RS274Interpreter:
        return self._command_set_coordinate_index(index)
    # TODO 其他坐标系指定
    
    # 当前坐标系原点设定
    def _coord_set_zero(self, x : bool = False, y : bool = False, z : bool = False, b : bool = False, c : bool = False, a : bool = False) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(x, bool)):
            raise InterpreterException(f"x type invalid: {type(x)} " + _get_stackmessage(2))
        if (not isinstance(y, bool)):
            raise InterpreterException(f"y type invalid: {type(y)} " + _get_stackmessage(2))
        if (not isinstance(z, bool)):
            raise InterpreterException(f"z type invalid: {type(z)} " + _get_stackmessage(2))
        if (not isinstance(b, bool)):
            raise InterpreterException(f"b type invalid: {type(b)} " + _get_stackmessage(2))
        if (not isinstance(c, bool)):
            raise InterpreterException(f"c type invalid: {type(c)} " + _get_stackmessage(2))
        if (not isinstance(a, bool)):
            raise InterpreterException(f"a type invalid: {type(a)} " + _get_stackmessage(2))

        set_zero_axis_list = [x, y, z, b, c, a]
        command = {
            CommandDictKey.CommandType.name: CommandType.CoordSetZeroCommand.name,
            CommandDictKey.SetZeroAxisList.name: set_zero_axis_list,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(2), # indirect call
            CommandDictKey.CommandStr.name: _get_caller_code(2)
        }
        
        self.__g_command_list.append(command)
        return self
    
    def coord_set_zero(self, x : bool = False, y : bool = False, z : bool = False, b : bool = False, c : bool = False, a : bool = False) -> RS274Interpreter:
        return self._coord_set_zero(x, y, z, b, c, a)
    
    def coord_set_x_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(x=True)
    def coord_set_y_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(y=True)
    def coord_set_z_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(z=True)
    def coord_set_b_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(b=True)
    def coord_set_c_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(c=True)
    def coord_set_a_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(a=True)
    def coord_set_all_zero(self) -> RS274Interpreter:
        return self._coord_set_zero(x=True, y=True, z=True, b=True, c=True, a=True)
    
    # 程序结束指令
    def m02(self) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        self.__g_environment.set_program_end_flag()
        command = {
            CommandDictKey.CommandType.name: CommandType.ProgramEndCommand.name,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self
    
    # 暂停指令
    def m00(self) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        command = {
            CommandDictKey.CommandType.name: CommandType.PauseCommand.name,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self

    @staticmethod
    def _get_coordinate_dict(x: float = None, y: float = None, z: float = None, b: float = None, c: float = None, a: float = None) -> list:
        # coordinates = {}
        coordinates = []
        if (x is not None):
            if(not isinstance(x, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: x, {type(x)} " + _get_stackmessage(2))
            # coordinates["x"] = x
            coordinates.append(x)
        else:
            # coordinates["x"] = None
            coordinates.append(None)
        
        if (y is not None):
            if(not isinstance(y, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: y, {type(y)} " + _get_stackmessage(2))
            # coordinates["y"] = y
            coordinates.append(y)
        else:
            # coordinates["y"] = None
            coordinates.append(None)
            
        if (z is not None):
            if(not isinstance(z, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: z, {type(z)} " + _get_stackmessage(2))
            # coordinates["z"] = z
            coordinates.append(z)
        else:
            # coordinates["z"] = None
            coordinates.append(None)
        
        if (b is not None):
            if(not isinstance(b, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: b, {type(b)} " + _get_stackmessage(2))
            # coordinates["b"] = b
            coordinates.append(b)
        else:
            # coordinates["b"] = None
            coordinates.append(None)
        
        if (c is not None):
            if(not isinstance(c, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: c, {type(c)} " + _get_stackmessage(2))
            # coordinates["c"] = c
            coordinates.append(c)
        else:
            # coordinates["c"] = None
            coordinates.append(None)
        
        if (a is not None):
            if(not isinstance(a, (int, float))):
                raise InterpreterException(f"Coordinate Value Type Not Valid: a, {type(a)} " + _get_stackmessage(2))
            # coordinates["a"] = a
            coordinates.append(a)
        else:
            # coordinates["a"] = None
            coordinates.append(None)
        
        return coordinates

    # G01普通直线加工指令
    def g01(self, x: float = None, y: float = None, z: float = None, b: float = None, c: float = None, a: float = None) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        self.__g_environment.set_motion_mode(MotionMode.G01)
        self._assert_environment_valid()
        command = {
            CommandDictKey.CommandType.name: CommandType.G01MotionCommand.name,
            CommandDictKey.CoordinateMode.name: self.__g_environment.get_coordinate_mode().name,
            CommandDictKey.MotionMode.name: self.__g_environment.get_motion_mode().name,
            CommandDictKey.CoordinateIndex.name: self.__g_environment.get_coordinate_index(),
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        coordinates = self._get_coordinate_dict(x, y, z, b, c, a)
        
        command[CommandDictKey.Coordinates.name] = coordinates
            
        self.__g_command_list.append(command)
        return self
    
    # G00快速直线运动
    def g00(self, x: float = None, y: float = None, z: float = None, b: float = None, c: float = None, a: float = None,
            m05: bool = False, touch: bool = False) -> RS274Interpreter:
        # m05: true=忽略接触感知
        # touch: true=碰边, 强制m05=False; Touch行为在于接触感知后不会视为错误, 自动清错并接着执行后续代码
        if (self.__g_environment.is_program_end()): 
            return self
        
        self.__g_environment.set_motion_mode(MotionMode.G00)
        self._assert_environment_valid()
        
        if (not isinstance(m05, bool)):
            raise InterpreterException(f"m05 Type Not Valid: {type(m05)} " + _get_stackmessage())
        if (not isinstance(touch, bool)):
            raise InterpreterException(f"touch Type Not Valid: {type(touch)} " + _get_stackmessage())

        if (touch):
            m05 = False # 碰边行为, 一定要开启接触感知
        
        command = {
            CommandDictKey.CommandType.name: CommandType.G00MotionCommand.name,
            CommandDictKey.CoordinateMode.name: self.__g_environment.get_coordinate_mode().name,
            CommandDictKey.MotionMode.name: self.__g_environment.get_motion_mode().name,
            CommandDictKey.CoordinateIndex.name: self.__g_environment.get_coordinate_index(),
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code(),
            CommandDictKey.M05IgnoreTouchDetect.name: m05,
            CommandDictKey.G00Touch.name: touch,
            CommandDictKey.FeedSpeed.name: self.__g_environment.get_feed_speed()
        }
        
        coordinates = self._get_coordinate_dict(x, y, z, b, c, a)
        
        command[CommandDictKey.Coordinates.name] = coordinates
            
        self.__g_command_list.append(command)
        return self
    
    @staticmethod
    def _get_G01GroupPoints(points: Points) -> list:
        ret = []
        for p in points.get_points():
            ret.append({
                CommandDictKey.CoordinateMode.name: p.get_coordinate_mode().name,
                CommandDictKey.Coordinates.name: p.get_point(),
                CommandDictKey.LineNumber.name: p.get_line_number(),
                CommandDictKey.CommandStr.name: p.get_code_str()
            })
        
        return ret
    
    def g01_group(self, points: Points) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(points, Points)):
            raise InterpreterException(f"Points Type Not Valid: {type(points)} " + _get_stackmessage())
        
        self.__g_environment.set_motion_mode(MotionMode.G01)
        self._assert_environment_valid()
        
        command = {
            CommandDictKey.CommandType.name: CommandType.G01GroupMotionCommand.name,
            # CommandDictKey.CoordinateMode.name: self.__g_environment.get_coordinate_mode().name,
            # CommandDictKey.MotionMode.name: self.__g_environment.get_motion_mode().name,
            CommandDictKey.CoordinateIndex.name: self.__g_environment.get_coordinate_index(),
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        command[CommandDictKey.G01GroupPoints.name] = self._get_G01GroupPoints(points)
        
        # test
        # print("** g01_group")
        # points.test_print()
        # print(json.dumps(command, indent=2, ensure_ascii=False))
        self.__g_command_list.append(command)
        
        return self
    
    # 进给率F设定
    def f(self, speed : int) -> RS274Interpreter:
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(speed, int)):
            raise InterpreterException(f"Feed Speed Type Not Valid: {type(speed)} " + _get_stackmessage())
        
        if (speed <= 0):
            raise InterpreterException(f"Feed Speed Value ({speed}) Out of Range " + _get_stackmessage())
        
        self.__g_environment.set_feed_speed(speed)
        
        command = {
            CommandDictKey.CommandType.name: CommandType.FeedSpeedSetCommand.name,
            CommandDictKey.FeedSpeed.name: speed,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self
    
    # 穿透检测参数设定(数据库方法) TODO
    
    # 打孔代码
    def drill(self, depth: float, holdtime: int = 1000, touch: bool = False, breakout: bool = False, back: bool = False, spindle_speed: float = None) -> RS274Interpreter:
        # depth: 打孔深度 毫米
        # holdtime: 打孔结束保持时间 毫秒
        # touch: 打孔前碰边
        # breakout: 打孔时穿透检测
        # back: 打孔后抬起到加工开始位置
        # spindle_speed: 主轴转速
        
        if (self.__g_environment.is_program_end()): 
            return self
        
        if (not isinstance(depth, (int, float))):
            raise InterpreterException(f"Drill Depth Type Not Valid: {type(depth)} " + _get_stackmessage())
        if (depth <= 0):
            raise InterpreterException(f"Drill Depth Value ({depth}) Out of Range " + _get_stackmessage())
        
        if (not isinstance(holdtime, int)):
            raise InterpreterException(f"Drill Holdtime Type Not Valid: {type(holdtime)} " + _get_stackmessage())
        if (holdtime < 0):
            raise InterpreterException(f"Drill Holdtime Value ({holdtime}) Out of Range " + _get_stackmessage())
        
        if (not isinstance(touch, bool)):
            raise InterpreterException(f"Drill Touch Type Not Valid: {type(touch)} " + _get_stackmessage())
        
        if (not isinstance(breakout, bool)):
            raise InterpreterException(f"Drill Breakout Type Not Valid: {type(breakout)} " + _get_stackmessage())
        
        if (not isinstance(back, bool)):
            raise InterpreterException(f"Drill Back Type Not Valid: {type(back)} " + _get_stackmessage())
        
        if (not isinstance(spindle_speed, (int, float, type(None))) ):
            raise InterpreterException(f"Drill Spindle Speed Type Not Valid: {type(spindle_speed)} " + _get_stackmessage())
        
        command = {
            CommandDictKey.CommandType.name: CommandType.DrillMotionCommand.name,
            CommandDictKey.MotionMode.name: MotionMode.DRILL.name,
            CommandDictKey.DrillDepth.name: depth,
            CommandDictKey.DrillHoldTime.name: holdtime,
            CommandDictKey.DrillTouch.name: touch,
            CommandDictKey.DrillBreakout.name: breakout,
            CommandDictKey.DrillBack.name: back,
            CommandDictKey.DrillSpindleSpeed.name: spindle_speed,
            CommandDictKey.LineNumber.name: _get_caller_linenumber(),
            CommandDictKey.CommandStr.name: _get_caller_code()
        }
        
        self.__g_command_list.append(command)
        return self

    # 获取已解析的命令列表
    def get_command_list(self) -> list:
        return self.__g_command_list

    # 获取命令列表的json字符串形式
    def get_command_list_jsonstr(self) -> str:
        return json.dumps(self.__g_command_list, indent=2, ensure_ascii=False)

    # def reset(self) -> None:
    #     self.__init__()
        

def _test():
    i = RS274Interpreter()
    
    try :
        i.g54()
        i.g90()
        i.g01(x=1, z=2)
        i.m00()
        i.g01(x=2)
        i.e(11)
        # i.e(-1)
        i.f(-1)
        i.g00(y=1)
        
        return i
        
    except Exception as e:
        print(f"exception: {e}")
        exit(0)    

if __name__ == "__main__":
    
    i = _test()
        
    j = i.get_command_list_jsonstr()
    print(j)
    print(type(j))
    # print(type(CoordinateMode))
    print(CoordinateMode.AbsoluteMode.name)
    # print(CoordinateMode.AbsoluteMode.AbsoluteMode)
    
    d = json.loads(j)
    # print(d[1]['y'] is None)
    
    print(isinstance(MotionMode.G01, MotionMode))