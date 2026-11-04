"""两个控制台进程共用同一份数据目录：分别登录 QQ 10001 / 10002，互发消息并检查对方收到。
用法：python console_two_window_e2e.py <SocialQS 可执行文件路径>"""
import re, subprocess, sys, tempfile, threading, time, queue, os

exe = sys.argv[1]
data = tempfile.mkdtemp(prefix="qs-e2e-")

def start(name):
    p = subprocess.Popen([exe, "--data", data], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, bufsize=0)
    out = queue.Queue()
    def reader():
        for line in iter(p.stdout.readline, b""):
            out.put(line.decode("utf-8", "replace"))
    threading.Thread(target=reader, daemon=True).start()
    return p, out

def send(p, text, pause=0.4):
    p.stdin.write((text + "\n").encode("utf-8")); p.stdin.flush(); time.sleep(pause)

def drain(q):
    lines = []
    while not q.empty(): lines.append(q.get())
    return "".join(lines)

a, qa = start("A"); time.sleep(1.0)
b, qb = start("B"); time.sleep(1.0)
for p, acct in ((a, "10001"), (b, "10002")):
    send(p, "1"); send(p, "1"); send(p, acct); send(p, "123456", 1.0)
outA, outB = drain(qa), drain(qb)
assert "欢迎回来，丘山" in outA, outA[-800:]
assert "欢迎回来，林然" in outB, outB[-800:]
assert "发现你已开通微信" in outA
# A：消息 → a(找好友) → 选林然（好友列表第1位按账号排序为10002）
send(a, "1"); send(a, "a", 0.6)
listing = drain(qa)
assert "10002" in listing and "● 在线" in listing, listing[-600:]
idx = re.search(r"(\d+)\. [^\n]*10002", listing).group(1)
send(a, idx, 0.6)
send(a, "你好，我是丘山（控制台A）", 1.5)
outB = drain(qb)
assert "新消息" in outB and "你好，我是丘山（控制台A）" in outB, outB[-600:]
# B 回复
send(b, "1"); send(b, "1", 0.6)  # 会话列表第1个 = 与丘山
send(b, "收到，林然回复", 1.5)
outA = drain(qa)
assert "林然：收到，林然回复" in outA, outA[-600:]
assert "已送达" in outA or "已发送" in outA
for p in (a, b):
    send(p, "/q"); send(p, "0"); send(p, "0", 0.6)
    p.wait(timeout=10)
print("E2E OK: 两个控制台窗口自动发现并双向收发成功；数据目录", data)
