import ctypes
import sys

if sys.platform == "win32":
    DLL = ctypes.CDLL("./python_test.dll")
elif sys.platform == "darwin":
    DLL = ctypes.CDLL("./libpython_test.dylib")
else:
    DLL = ctypes.CDLL("./libpython_test.so")



class Component:
    VTYPE_EMPTY = 0
    VTYPE_NULL = 1
    VTYPE_I2 = 2
    VTYPE_I4 = 3
    VTYPE_R4 = 4
    VTYPE_R8 = 5
    VTYPE_DATE = 6
    VTYPE_TM = 7
    VTYPE_PSTR = 8
    VTYPE_INTERFACE = 9
    VTYPE_ERROR = 10
    VTYPE_BOOL = 11
    VTYPE_VARIANT = 12
    VTYPE_I1 = 13
    VTYPE_UI1 = 14
    VTYPE_UI2 = 15
    VTYPE_UI4 = 16
    VTYPE_I8 = 17
    VTYPE_UI8 = 18
    VTYPE_INT = 19
    VTYPE_UINT = 20
    VTYPE_HRESULT = 21
    VTYPE_PWSTR = 22
    VTYPE_BLOB = 23
    VTYPE_CLSID = 24

    def __init__(self, name=""):
        self.con = DLL.create_connection(name.encode())
        DLL.get_result_double.restype = ctypes.c_double
        self._ex()
        self._errors = None
        self.set_raise()

    def __del__(self):
        if self.con:
            DLL.delete_connection(self.con)

    def _read_string(self, proc, args=[]):
        sz = 1024
        st = ctypes.create_string_buffer(sz)
        nargs = args+[st, sz]
        rsz = proc(*nargs)
        if rsz > sz:
            st = ctypes.create_string_buffer(rsz)
            nargs = args+[st, rsz]
            proc(*nargs)
        return st.value.decode()

    def _ex(self):
        ex = ctypes.create_string_buffer(1024)
        ex = self._read_string(DLL.last_exception)
        if ex:
            raise RuntimeError(ex)

    def _fill_arguments(self, args, callback, **kwargs):
        arg = DLL.create_arguments(self.con)
        self._ex()
        try:
            ref_args = {}
            for i, x in enumerate(args):
                if isinstance(x, list):
                    ref_args[i] = x
                    x = x[0]
                if isinstance(x, bool):
                    DLL.add_argument_bool(arg, x)
                    self._ex()
                elif isinstance(x, int):
                    if "longs" in kwargs and i in kwargs["longs"]:
                        DLL.add_argument_long(arg, ctypes.c_longlong(x))
                    else:
                        DLL.add_argument_int(arg, x)
                    self._ex()
                elif isinstance(x, float):
                    DLL.add_argument_double(arg, ctypes.c_double(x))
                    self._ex()
                elif isinstance(x, str):
                    DLL.add_argument_str(arg, x.encode())
                    self._ex()
                elif x is None:
                    DLL.add_argument_null(arg)
                    self._ex()
                else:
                    raise RuntimeError(f"Unsupported argument type {type(x)}")
            ret = callback(arg)
            self._ex()
            for i, x in ref_args.items():
                ires = DLL.create_argument_result(arg, i)
                self._ex()
                try:
                    x[0] = self._get_result(ires)
                finally:
                    DLL.delete_result(ires)
            return ret
        finally:
            DLL.delete_arguments(arg)

    def call_proc(self, name, *args, **kwargs):
        def call(arg):
            return DLL.call_proc(self.con, name.encode(), arg)
        return self._fill_arguments(args, call, **kwargs)

    def call_func(self, name, *args, **kwargs):
        def call(arg):
            ires = ctypes.c_int()
            ret = DLL.call_func(self.con, name.encode(), arg, ctypes.byref(ires))
            result = None
            if ret:
                try:
                    result = self._get_result(ires.value)
                finally:
                    DLL.delete_result(ires.value)
            return (ret, result)
        return self._fill_arguments(args, call, **kwargs)

    def set_prop(self, name, value, **kwargs):
        def call(arg):
            return DLL.set_prop(self.con, name.encode(), arg)
        return self._fill_arguments([value], call, **kwargs)

    def get_prop(self, name, **kwargs):
        ires = ctypes.c_int()
        ret = DLL.get_prop(self.con, name.encode(), ctypes.byref(ires))
        result = None
        if ret:
            try:
                result = self._get_result(ires.value)
            finally:
                DLL.delete_result(ires.value)
        return (ret, result)

    def _get_result(self, ires):
        tp = DLL.get_result_type(ires)
        self._ex()
        if tp == self.VTYPE_EMPTY or tp == self.VTYPE_NULL:
            return None
        if tp in (self.VTYPE_I1, self.VTYPE_I2, self.VTYPE_I4, self.VTYPE_I8, self.VTYPE_INT):
            ret = DLL.get_result_int(ires)
            self._ex()
            return ret
        if tp in (self.VTYPE_UI1, self.VTYPE_UI2, self.VTYPE_UI4, self.VTYPE_UI8, self.VTYPE_UINT):
            ret = DLL.get_result_uint(ires)
            self._ex()
            return ret
        if tp in (self.VTYPE_R4, self.VTYPE_R8, self.VTYPE_DATE):
            ret = DLL.get_result_double(ires)
            self._ex()
            return ret
        if tp == self.VTYPE_BOOL:
            ret = True if DLL.get_result_bool(ires) else False
            self._ex()
            return ret
        if tp == self.VTYPE_PWSTR:
            ret = self._read_string(DLL.get_result_str, [ires])
            self._ex()
            return ret
        raise RuntimeError(f"Unsupported result type {tp}")

    def set_raise(self, is_raise=True):
        DLL.set_raise(self.con, is_raise)
        self._ex()

    def clear_errors(self):
        self._errors = None

    def get_errors(self):
        if not self._errors:
            cnt = DLL.errornum(self.con)
            self._ex()
            self._errors = []
            for i in range(cnt):
                self._errors.append(self._read_string(DLL.get_error, [self.con, i]))
                self._ex()
        return self._errors

    def get_last_error(self):
        (res, err) = self.call_func("GetLastError")
        if not res:
            raise RuntimeError("Get last error returns false")
        return err

    def get_events(self):
        ret = []
        cnt = DLL.eventnum(self.con)
        self._ex()
        for i in range(cnt):
            ret.append(self._read_string(DLL.get_event, [self.con, i]))
            self._ex()
        DLL.clear_events(self.con, cnt)
        self._ex()
        return ret

    def test_default_params(self, name, count):
        DLL.test_default_params(self.con, name.encode(), count)
        self._ex()
