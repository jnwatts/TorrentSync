from datetime import datetime

def Log(msg, *args):
    print("[%s] %s" % (str(datetime.now()), msg), args)
