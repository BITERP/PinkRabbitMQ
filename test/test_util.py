from amqp import *
import pytest
import os

def create():
    return Component("PinkRabbitMQ")

def test_connect():
    connect()

@pytest.mark.skipif(not os.getenv("PRMQ_TEST_SSL"), reason="Skip ssl tests")
def test_connect_ssl():
    connect(ssl=True)

@pytest.mark.skipif(not os.getenv("PRMQ_TEST_SSL"), reason="Skip ssl tests")
def test_connect_ssl2():
    connect(ssl=True)

def test_connect_fail():
    try:
        connect(login="admin")
        raise Exception("Must not be here")
    except RuntimeError as e:
        assert "Wrong login, password or vhost" in str(e)


def test_defparams():
    com = create()
    com.test_default_params("Connect", 5)
    com.test_default_params("Connect", 5)
    com.test_default_params("DeclareQueue", 5)
    com.test_default_params("DeclareExchange", 5)
    com.test_default_params("BasicPublish", 5)
    com.test_default_params("BindQueue", 3)

def test_version():
    com = create()
    res, val = com.get_prop("Version")
    assert res
    print(f"Version is {val}")

def test_set_props():
    com = connect()
    res, val = com.get_prop("CorrelationId")
    assert res
    assert val != "MY_CORR_ID"
    res = com.set_prop("CorrelationId", "MY_CORR_ID")
    assert res
    publish(com, "test_queue", "Test CORRID")
    receive(com, "test_queue", "Test CORRID")
    res, val = com.get_prop("CorrelationId")
    assert res
    assert val == "MY_CORR_ID"

def test_priority():
    com = connect()
    res = com.call_proc("SetPriority", 13)
    publish(com, "test_queue", "Test Priority")
    receive(com, "test_queue", "Test Priority")
    res, ret = com.call_func("GetPriority")
    assert res
    assert ret == 13


