from amqp import *

def test_ext_bad_json():
    com = connect()
    try:
        del_queue(com, QUEUE)
        (res, ret) = com.call_func("DeclareQueue", QUEUE, False, True, False, False, 0, "NOT JSON")
        raise Exception("Must not be here")
    except RuntimeError as e:
        assert "syntax error"


def test_ext_good_param():
    com = connect()
    make_queue(com, QUEUE, {"x-message-ttl":60000})

def test_ext_exchange():
    com = connect()
    make_exchange(com, QUEUE, {"alternate-exchange":"myae"})

def test_ext_bind():
    com = connect()
    bind_queue(com, QUEUE, {"x-message-ttl":60000})

def test_ext_publish():
    com = connect()
    publish(com, QUEUE, "args message", {"some-header":13,"yes":True,"no":False})
    receive(com, QUEUE, "args message")
    res, ret = com.call_func("GetHeaders")
    assert res
    hdr = json.loads(ret)
    assert hdr['some-header'] == 13
    assert hdr['yes']
    assert not hdr['no']
