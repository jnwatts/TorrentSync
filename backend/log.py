from datetime import datetime

def Log(msg, *args):
    if args:
        print("[%s] %s" % (str(datetime.now()), msg), args)
    else:
        print("[%s] %s" % (str(datetime.now()), msg))
