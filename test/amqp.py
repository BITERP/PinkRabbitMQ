import logging
import json
import os

logger = logging.getLogger("AMQP")

from addin1c import Component

QUEUE = "test_queue"

def connect(host=None, port=None, login="guest", pswd="guest", vhost="/", ssl=False):
    host = host or os.getenv("RMQ_HOST", "127.0.0.1")
    port = port or (5671 if ssl else 5672)
    vhost = vhost or login
    com = Component("PinkRabbitMQ")
    res = com.call_proc("Connect", host, port, login, pswd, vhost, 0, ssl, 5)
    assert res
    return com

def del_queue(com, queue):
    res = com.call_proc("DeleteQueue", queue, False, False)
    assert res

def make_queue(com, queue, props=None):
    del_queue(com, queue)
    (res, ret) = com.call_func("DeclareQueue", queue, False, True, False, False, 0, json.dumps(props) if props else None)
    assert res
    return ret

def del_exchange(com, exch):
    res = com.call_proc("DeleteExchange", exch, False)
    assert res

def make_exchange(com, exch, props=None):
    del_exchange(com, exch)
    res = com.call_proc("DeclareExchange", exch, "topic", False, True, False, json.dumps(props) if props else None)
    assert res

def bind_queue(com, name, props=None):
    make_queue(com, name)
    make_exchange(com, name)
    res = com.call_proc("BindQueue", name, name, "#", json.dumps(props) if props else None)
    assert res

def publish(com, queue, message, props=None, no_bind=False):
    if not no_bind:
        bind_queue(com, queue)
    res = com.call_proc("BasicPublish", queue, queue, message, 0, False, json.dumps(props) if props else None)
    assert res

def consume(com, queue, size=10):
    res, ret = com.call_func("BasicConsume", queue, "", False, False, size, None)
    assert res
    return ret

def receive(com, queue, message):
    ctag = consume(com, queue)
    msg = [""]
    mtag = [0]
    while True:
        res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 10000)
        assert res
        if not ret:
            return None
        if msg[0] == message:
            return (msg[0], mtag[0])
        com.call_proc("BasicAck", mtag[0])

