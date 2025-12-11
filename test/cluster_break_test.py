#!/usr/bin/env python3

import logging
import argparse
import sys
import json
import time
logger = logging.getLogger("CLUSTER")

from amqp import *

QUEUE_EX_NAME = "test_cluster"

def connect(cfg):
    com = Component("PinkRabbitMQ")
    res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
    assert res
    return com

def run(opts):
    with open(opts.config) as f:
        config = json.load(f)
    com = connect(config)
    publish(com, QUEUE_EX_NAME, "Cluster message 0")
    ctag = consume(com, QUEUE_EX_NAME)
    msg = ["msg"]
    mtag = [1]
    i = 1
    while True:
        try:
            res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 1000)
        except RuntimeError as e:
            print(f"consume error: {e}")
        if ret:
            print(f"Message consumed: {msg[0]}")
            com.call_proc("BasicAck", mtag[0])
        else:
            print("Message not consumed")
        message = f"Cluster message {i}"
        i += 1
        com.call_proc("BasicPublish", QUEUE_EX_NAME, QUEUE_EX_NAME, message, 0, False, None)
        time.sleep(1)

def main():
    args =argparse.ArgumentParser()
    args.add_argument("--verbose", "-v", action="store_true")
    args.add_argument("--config", "-c", default="cluster_conf.json")
    opts = args.parse_args()
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG if opts.verbose else logging.INFO)
    while True:
        try:
            run(opts)
        except RuntimeError as e:
            print(f"Loop error: {e}")


if __name__ == "__main__":
    main()
