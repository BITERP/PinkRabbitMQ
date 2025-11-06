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

def failed_connect(err, **kwargs):
    if not isinstance(err, (tuple, list)):
        err = [err]
    try:
        connect(**kwargs)
        raise Exception("Must not be here")
    except RuntimeError as e:
        found = [x in str(e) for x in err]
        assert True in found, f"Errors {err} not found in {e}"

def test_connect_fail():
    failed_connect("Login was refused", login="admin")
    failed_connect("Wrong hostname", host="unexistent.internal")
    failed_connect("Wrong hostname", host="unexistent.internal", ssl=True)
    failed_connect(["connection timed out", "Wrong hostname"], host="testhost")
    failed_connect(["connection timed out", "Wrong hostname"], host="testhost", ssl=True)
    failed_connect("connection timed out", host="172.16.3.254")
    failed_connect("connection timed out", host="172.16.3.254", ssl=True)
    failed_connect("Connection refused", host="localhost")
    failed_connect("Connection refused", host="localhost", ssl=True)


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
