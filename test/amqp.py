import logging
import json
import os

logger = logging.getLogger("AMQP")

from addin1c import Component

QUEUE = "test_queue"

def get_config(host, port, login, pswd, vhost, ssl):
    if os.path.exists("test.conf"):
        with open("test.conf") as f:
            cfg = json.load(f)
        if 'host' in cfg and not host:
            host = cfg['host']
        if 'login' in cfg and not login:
            login = cfg['login']
        if 'password' in cfg and not pswd:
            pswd = cfg['password']
        if 'vhost' in cfg and not vhost:
            vhost = cfg['vhost']
    if not vhost:
        vhost = login or "/"
    return {
        'host': host or os.getenv("RMQ_HOST", "127.0.0.1"),
        'port': port or (5671 if ssl else 5672),
        'login': login or "guest",
        'pswd': pswd or "guest",
        'vhost': vhost,
        'ssl': ssl
    }

def connect(host=None, port=None, login=None, pswd=None, vhost=None, ssl=False):
    cfg = get_config(host, port, login, pswd, vhost, ssl)
    com = Component("PinkRabbitMQ")
    res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
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

