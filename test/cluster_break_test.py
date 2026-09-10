#!/usr/bin/env python3

import argparse
import json
import logging
import sys
import time

logger = logging.getLogger("CLUSTER")

from amqp import *

QUEUE_EX_NAME = "test_queue"

def connect(cfg):
    com = Component("PinkRabbitMQ")
    #res = com.set_prop("UseAddError", True)
    #assert res
    res = com.call_proc("Connect", cfg['host'], cfg['port'], cfg['login'], cfg['pswd'], cfg['vhost'], 0, cfg['ssl'], 5)
    assert res
    return com

def run(opts):
    if opts.stdin:
        line = sys.stdin.read()
        tok = json.loads(line)
        config = {
            'host': tok['server'],
            'port': tok['amqps_port'],
            'login': '',
            'pswd': tok['access_token'],
            'vhost': "ai",
            'ssl': True
        }
    else:
        with open(opts.config) as f:
            config = json.load(f)

    com = connect(config)
    res, ret = com.call_func("DeclareQueue", QUEUE_EX_NAME, False, True, False, False, 0, None)
    if not res:
        print(f"Declare queue error: {com.get_last_error()}")
    ctag = consume(com, QUEUE_EX_NAME)
    msg = ["msg"]
    mtag = [1]
    i = 1
    while True:
        # try:
        #     res, ret = com.call_func("BasicConsumeMessage", ctag, msg, mtag, 500)
        #     if not res:
        #         print(f"consume error: {com.get_last_error()}")
        #     if ret:
        #         print(f"Message consumed: {msg[0]}")
        #         com.call_proc("BasicAck", mtag[0])
        #     else:
        #         print("Message not consumed")
        # except Exception as e:
        #     print(f"Consume Exception: {e}")
        #     time.sleep(5)
        i += 1
        try:
            res, ret = com.call_func("DeclareQueue", QUEUE_EX_NAME, True, True, False, False, 0, None)
            if not res:
                print(f"Declare queue error: {com.get_last_error()}")
        except Exception as e:
            print(f"Declare Exception: {e}")
            time.sleep(5)


def main():
    args =argparse.ArgumentParser()
    args.add_argument("--verbose", "-v", action="store_true")
    args.add_argument("--config", "-c", default="cluster_conf.json")
    args.add_argument("--stdin", "-i", action="store_true")
    opts = args.parse_args()
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG if opts.verbose else logging.INFO)
    #try:
    run(opts)
    #except RuntimeError as e:
    #    print(f"Loop error: {e}")


if __name__ == "__main__":
    main()
