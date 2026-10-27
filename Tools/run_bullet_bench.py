# 탄막 측정 자동 실행: 단독 실행(-game)으로 방식 × 탄 수 × 3회 → 로그에서 숫자를 모아 평균 표
# 사용 (에디터를 끈 상태 권장, 엔진 파이썬):
#   "D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" -I -X utf8 Tools/run_bullet_bench.py
# 한 판: L_Dev, 1600×900 창, 3초 뒤 BenchBullets 탄수 방식 10초 → 끝나면 종료 (-BenchQuit). 전투는 시작하지 않음
# 결과: Saved/Bench/bullets_날짜.md (판마다 로그는 같은 폴더)
# 방식 하나만: 끝에 0(탄 매니저) 또는 1(같은 외형 액터)
import datetime
import os
import re
import statistics
import subprocess
import sys

ENGINE = r"D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
PROJECT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "TwinSwift.uproject"))
OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "Saved", "Bench"))

COUNTS = [1000, 2000, 3000]
MODES = [(0, "탄 매니저"), (1, "같은 외형 액터")]
RUNS = 3
SECONDS = 10

PATTERNS = {
    "frame": re.compile(r"\[벤치\] 프레임: \d+개, 평균 ([\d.]+) ms, 상위1% ([\d.]+) ms, 최대 ([\d.]+) ms"),
    "game": re.compile(r"\[탄막 벤치\] 게임 스레드 평균: ([\d.]+) ms"),
    "manager": re.compile(r"\[탄막 벤치\] 탄 매니저 처리: 평균 ([\d.]+) ms"),
    "gc": re.compile(r"\[벤치\] GC: (\d+)번, 합계 ([\d.]+) ms"),
    # 액터 방식: 목표 탄 수가 실제로 떠 있었는지 (탄끼리 부딪혀 사라지면 비교가 안 됨)
    "alive": re.compile(r"떠 있는 액터 탄: 평균 (\d+)발"),
}


def run_once(count, mode, index, stamp):
    log = os.path.join(OUT_DIR, f"bullets_{stamp}_m{mode}_{count}_{index}.log")
    args = [ENGINE, PROJECT, "/Game/Maps/L_Dev", "-game", "-windowed", "-resx=1600", "-resy=900",
            f"-BenchBullets={count},{mode},{SECONDS}", "-BenchQuit", "-nosplash", "-unattended", f"-abslog={log}"]
    print(f"  실행: 방식 {mode}, {count}발, {index}회", flush=True)
    subprocess.run(args, timeout=300)
    text = open(log, encoding="utf-8", errors="ignore").read() if os.path.exists(log) else ""
    result = {}
    for key, pattern in PATTERNS.items():
        match = pattern.search(text)
        if match:
            result[key] = [float(g) for g in match.groups()]
    if "frame" not in result:
        print(f"    결과 없음 (로그: {log})")
    return result


def mean(values):
    return f"{statistics.mean(values):.2f}" if values else "-"


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M")
    rows = []
    modes = [m for m in MODES if len(sys.argv) < 2 or str(m[0]) == sys.argv[1]]
    for mode, label in modes:
        for count in COUNTS:
            runs = [run_once(count, mode, i + 1, stamp) for i in range(RUNS)]
            runs = [r for r in runs if "frame" in r]
            frame = [r["frame"][0] for r in runs]
            alive = [r["alive"][0] for r in runs if "alive" in r]
            top1 = [r["frame"][1] for r in runs]
            game = [r["game"][0] for r in runs if "game" in r]
            manager = [r["manager"][0] for r in runs if "manager" in r]
            each = " / ".join(f"{v:.2f}" for v in frame)
            rows.append(f"| {label} | {count} | {len(runs)} | {mean(frame)} ({each}) | {mean(top1)} | {mean(game)} | {mean(manager)} | {mean(alive)} |")
            print(rows[-1], flush=True)

    header = [
        f"# 탄막 측정 ({stamp}, 단독 실행 1600×900, {SECONDS}초, 판마다 {RUNS}회 평균)",
        "",
        "| 방식 | 탄 수 | 성공 | 프레임 평균 ms (각 회) | 상위 1% ms | 게임 스레드 ms | 매니저 처리 ms | 떠 있는 액터 탄 |",
        "|---|---|---|---|---|---|---|---|",
    ]
    path = os.path.join(OUT_DIR, f"bullets_{stamp}.md")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(header + rows) + "\n")
    print("결과:", path)


if __name__ == "__main__":
    sys.exit(main())
