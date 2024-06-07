import pytest
from amqp import *

@pytest.fixture
def com():
    com = connect()
    bind_queue(com, QUEUE)
    yield com


def test_declare_exchange():
    com = connect()
    make_exchange(com, "mk_exch")

def test_delete_exchange():
    com = connect()
    make_exchange(com, "mk_exch")
    del_exchange(com, "mk_exch")

def test_declare_queue():
    com = connect()
    make_queue(com, "mk_queue")

def test_delete_queue():
    com = connect()
    make_queue(com, "mk_queue")
    del_queue(com, "mk_queue")

def test_bind_queue():
    com = connect()
    bind_queue(com, "bind_queue")

def test_unbind_queue():
    com = connect()
    bind_queue(com, "ubind_queue")
    res = com.call_proc("UnbindQueue", "ubind_queue", "ubind_queue", "#")

def test_publish(com):
    res = com.call_proc("BasicPublish", QUEUE, QUEUE, "Test Message", 0, False, None)
    assert res

def test_consume(com):
    bind_queue(com, QUEUE)
    ctag = consume(com, QUEUE)
    assert len(ctag)>0

def test_ack(com):
    msg = "ack test"
    publish(com, QUEUE, msg)
    msg, tag = receive(com, QUEUE, msg)
    res = com.call_proc("BasicAck", tag)
    assert res

def test_nack(com):
    msg = "nack test"
    publish(com, QUEUE, msg)
    msg, tag = receive(com, QUEUE, msg)
    res = com.call_proc("BasicReject", tag)
    assert res

def test_cancel(com):    
    ctag = consume(com, QUEUE)
    assert len(ctag) > 0
    res = com.call_proc("BasicCancel", ctag)
    assert res

def test_consume_msg(com):
    publish(com, "constest", "Test Consume Msg")
    ctag = consume(com, "constest")
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
    assert res
    assert ret
    assert msg[0] == "Test Consume Msg"
    assert mtag[0] > -1

def test_consume_nomsg(com):
    ctag = consume(com, QUEUE)
    msg = ["msg"]
    mtag = [1]
    while True:
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 100)
        if not ret:
            break
    assert res
    assert msg[0] == ''
    assert mtag[0] == 0

