import json
import pytest
from amqp import *
from addin1c import Component

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

def test_declare_queue_twice():
    com = connect()
    make_queue(com, "mk_queue_2")
    try:
        (res, ret) = com.call_func("DeclareQueue", "mk_queue_2", False, False, False, True, 0, None)
        raise Exception("Must not be here")
    except RuntimeError as e:
        assert "PRECONDITION_FAILED" in str(e)

def test_delete_queue():
    com = connect()
    make_queue(com, "mk_queue")
    del_queue(com, "mk_queue")

def test_bind_queue():
    com = connect()
    bind_queue(com, "bind_queue")

def test_bind_unexistent():
    com = connect()
    make_exchange(com, "bunx_exch")
    try:
        res = com.call_proc("BindQueue", "unexistent", "bunx_exch", "#", None)
        raise Exception("Must not be here")
    except RuntimeError as e:
        assert "NOT_FOUND" in str(e)


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
    res = com.call_proc("BasicReject", tag, False)
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
    assert msg[0] == None
    assert mtag[0] == 0

def test_batch_publish(com):
    Q = "batch"
    msgs = [
        {"routingKey":"#", "body":"message1"},
        {"routingKey":"#", "body":"message2", "properties":{"AppId":"MYAPP","MessageId":"message2"}, "headers":{"some-header":13}},
        {"routingKey":"#", "body":"message3", "properties":{"AppId":"MYAPP","MessageId":"message3"}, "headers":{"some-header":13}},
    ]
    bind_queue(com, Q)
    res = com.call_proc("BatchPublish", Q, False, json.dumps(msgs))
    assert res
    ctag = consume(com, Q)
    for i in range(3):
        msg = [""]
        mtag = [-1]
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
        assert res
        assert ret
        assert msg[0] == "message" + str(i+1)
        res, val = com.get_prop("MessageId")
        assert res
        assert i == 0 or val == msg[0]
        res, val = com.get_prop("AppId")
        assert res
        assert i == 0 or val == "MYAPP"
        if i > 0:
            res, ret = com.call_func("GetHeaders")
            assert res
            hdr = json.loads(ret)
            assert hdr['some-header'] == 13
        com.call_proc("BasicAck", mtag[0])

def test_queue_message_count(com):
    Q = "count_queue"
    bind_queue(com, Q)
    res, count = com.call_func("GetQueueMessageCount", Q)
    assert res
    assert count == 0
    publish(com, Q, "one", no_bind=True)
    res, count = com.call_func("GetQueueMessageCount", Q)
    assert res
    assert count == 1
    publish(com, Q, "two", no_bind=True)
    res, count = com.call_func("GetQueueMessageCount", Q)
    assert res
    assert count == 2

def test_consume_missing_queue(com):
    try:
        res, tag = com.call_func("BasicConsume", "no_such_queue_xyz", "", False, False, 0, None)
        raise Exception("Must not succeed")
    except RuntimeError as e:
        assert "NOT_FOUND" in str(e) or "no_such_queue" in str(e).lower()

def test_publish_missing_exchange(com):
    try:
        com.call_proc("BasicPublish", "no_such_exchange_xyz", "rk", "msg", 0, False, None)
        raise Exception("Must not succeed")
    except RuntimeError as e:
        err1 = str(e)
    assert "NOT_FOUND" in err1 or "no_such_exchange" in err1.lower()
    # Channel may be closed by broker after NOT_FOUND; reopen must still report the same error.
    try:
        com.call_proc("BasicPublish", "no_such_exchange_xyz", "rk", "msg2", 0, False, None)
        raise Exception("Must not succeed")
    except RuntimeError as e:
        err2 = str(e)
    assert "NOT_FOUND" in err2 or "no_such_exchange" in err2.lower()

def test_publish_large_message(com):
    Q = "large_msg"
    bind_queue(com, Q)
    body = "x" * 130000
    res = com.call_proc("BasicPublish", Q, Q, body, 0, False, None)
    assert res
    ctag = consume(com, Q)
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
    assert res and ret
    assert len(msg[0]) == 130000
    com.call_proc("BasicAck", mtag[0])

def test_publish_while_consuming(com):
    Q = "pub_while_consume"
    bind_queue(com, Q)
    ctag = consume(com, Q)
    res = com.call_proc("BasicPublish", Q, Q, "hello while consuming", 0, False, None)
    assert res
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
    assert res and ret
    assert msg[0] == "hello while consuming"
    com.call_proc("BasicAck", mtag[0])

def test_publish_consume_400kb(com):
    Q = "msg400kb"
    bind_queue(com, Q)
    body = "x" * 410000
    res = com.call_proc("BasicPublish", Q, Q, body, 0, False, None)
    assert res
    ctag = consume(com, Q)
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 30000)
    assert res and ret
    assert len(msg[0]) == 410000
    com.call_proc("BasicAck", mtag[0])

def test_many_messages_manual_ack(com):
    Q = "stress_ack"
    bind_queue(com, Q)
    for i in range(100):
        res = com.call_proc("BasicPublish", Q, Q, f"msg{i}", 0, False, None)
        assert res
    ctag = consume(com, Q)
    for i in range(100):
        msg = [""]
        mtag = [-1]
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
        assert res and ret, f"failed at message {i}"
        com.call_proc("BasicAck", mtag[0])
    com.call_proc("BasicCancel", "")

def test_many_messages_reconnect(com):
    Q = "stress_reconnect"
    bind_queue(com, Q)
    for i in range(50):
        com.call_proc("BasicPublish", Q, Q, f"body{i}", 0, False, None)
    ctag = consume(com, Q)
    for i in range(50):
        msg = [""]
        mtag = [-1]
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
        assert res and ret
        com.call_proc("BasicAck", mtag[0])
    from amqp import get_config
    cfg = get_config(None, None, None, None, None, False)
    res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
    assert res

def test_reconnect():
    """Regression for #79: repeated Connect on the same component instance."""
    from amqp import get_config
    cfg = get_config(None, None, None, None, None, False)
    com = Component("PinkRabbitMQ")
    for i in range(5):
        res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
        assert res, f"Connect failed on attempt {i}"
        name = f"reconn_q_{i}"
        res, _ = com.call_func("DeclareQueue", name, False, True, False, False, 0, None)
        assert res, f"DeclareQueue failed on attempt {i}"
        com.call_proc("DeleteQueue", name, False, False)

def test_reconnect_with_workload():
    """Regression for #89: repeated Connect after publish/consume must succeed."""
    from amqp import get_config
    cfg = get_config(None, None, None, None, None, False)
    Q = "reconnect_work"
    for cycle in range(3):
        com = Component("PinkRabbitMQ")
        res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
        assert res, f"connect failed at cycle {cycle}"
        bind_queue(com, Q)
        publish(com, Q, f"cycle{cycle}", no_bind=True)
        ctag = consume(com, Q, size=0)
        msg = [""]
        mtag = [-1]
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
        assert res and ret, f"consume failed at cycle {cycle}"
        assert msg[0] == f"cycle{cycle}"
        com.call_proc("BasicAck", mtag[0])
        com.call_proc("BasicCancel", ctag)
        res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
        assert res, f"reconnect failed at cycle {cycle}"

def test_connect_immediate_declare_queue():
    """Regression for #79/#65: DeclareQueue right after Connect without pause."""
    from amqp import get_config
    cfg = get_config(None, None, None, None, None, False)
    for i in range(10):
        com = Component("PinkRabbitMQ")
        res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
        assert res
        name = f"imm_queue_{i}"
        res, _ = com.call_func("DeclareQueue", name, False, True, False, False, 0, None)
        assert res
        com.call_proc("DeleteQueue", name, False, False)

def test_connect_immediate_declare():
    """Regression for #65: DeclareExchange right after Connect without pause."""
    from amqp import get_config
    cfg = get_config(None, None, None, None, None, False)
    for i in range(10):
        com = Component("PinkRabbitMQ")
        res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
        assert res
        name = f"imm_exch_{i}"
        res = com.call_proc("DeclareExchange", name, "topic", False, True, False, None)
        assert res
        com.call_proc("DeleteExchange", name, False)

def test_quorum_queue_consume(com):
    """Regression for #90: quorum queues reject global QoS."""
    Q = "quorum_queue"
    del_queue(com, Q)
    res, _ = com.call_func("DeclareQueue", Q, False, True, False, False, 0, json.dumps({"x-queue-type": "quorum"}))
    assert res
    make_exchange(com, Q)
    res = com.call_proc("BindQueue", Q, Q, "#", None)
    assert res
    publish(com, Q, "quorum-msg", no_bind=True)
    # selectSize=0 must not send basic.qos (works with quorum)
    res, ctag = com.call_func("BasicConsume", Q, "", False, False, 0, None)
    assert res
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
    assert res and ret
    assert msg[0] == "quorum-msg"
    com.call_proc("BasicAck", mtag[0])
    com.call_proc("BasicCancel", ctag)
    # selectSize>0 with per-consumer QoS must also work
    publish(com, Q, "quorum-msg-2", no_bind=True)
    res, ctag = com.call_func("BasicConsume", Q, "", False, False, 10, None)
    assert res
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
    assert res and ret
    assert msg[0] == "quorum-msg-2"
    com.call_proc("BasicAck", mtag[0])
    com.call_proc("BasicCancel", ctag)

def test_consume_message_returns_without_full_timeout(com):
    """Regression for #78: message already buffered must not wait full timeout."""
    import time
    Q = "fast_consume"
    bind_queue(com, Q)
    publish(com, Q, "instant", no_bind=True)
    ctag = consume(com, Q, size=0)
    start = time.monotonic()
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 30000)
    elapsed = time.monotonic() - start
    assert res and ret
    assert msg[0] == "instant"
    assert elapsed < 5.0, f"BasicConsumeMessage waited {elapsed:.1f}s with message ready"
    com.call_proc("BasicAck", mtag[0])
    com.call_proc("BasicCancel", ctag)

def test_routing_key_too_long(com):
    """Regression for #82: routing key longer than 255 bytes must fail clearly."""
    bind_queue(com, QUEUE)
    long_key = "k" * 300
    try:
        com.call_proc("BasicPublish", QUEUE, long_key, "body", 0, False, None)
        raise Exception("Must not succeed")
    except RuntimeError as e:
        assert "255" in str(e)

def test_publish_while_consuming(com):
    """Regression for #51: publish while read channel has active consumer."""
    Q = "pub_while_consume"
    bind_queue(com, Q)
    for i in range(3):
        publish(com, Q, f"pre{i}", no_bind=True)
    res, ctag = com.call_func("BasicConsume", Q, "", False, False, 0, None)
    assert res
    for i in range(15):
        res = com.call_proc("BasicPublish", Q, Q, f"while{i}", 0, False, None)
        assert res, f"publish while consuming failed at {i}"
    received = 0
    for _ in range(20):
        msg = [""]
        mtag = [-1]
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 3000)
        if not res or not ret:
            break
        received += 1
        com.call_proc("BasicAck", mtag[0])
    assert received >= 15
    com.call_proc("BasicCancel", ctag)

def test_consume_empty_queue_no_throw(com):
    """Regression for #98: empty queue must return False, not crash."""
    Q = "empty_consume"
    bind_queue(com, Q)
    res, ctag = com.call_func("BasicConsume", Q, "", False, False, 0, None)
    assert res
    msg = [""]
    mtag = [0]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 500)
    assert res
    assert not ret
    res, err = com.call_func("GetLastError")
    assert res
    assert err
    com.call_proc("BasicCancel", ctag)

def test_consume_short_timeout_then_large_message(com):
    """Regression for #93: short timeout must not break subsequent large message read."""
    Q = "large_timeout"
    bind_queue(com, Q)
    ctag = consume(com, Q, size=0)
    msg = [""]
    mtag = [-1]
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 50)
    assert res
    assert not ret
    body = "y" * 500000
    publish(com, Q, body, no_bind=True)
    res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 60000)
    assert res and ret
    assert len(msg[0]) == 500000
    com.call_proc("BasicAck", mtag[0])
    com.call_proc("BasicCancel", ctag)
