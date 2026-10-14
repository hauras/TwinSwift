# 실행 중인 언리얼 에디터에 Python 코드를 보내 실행
# 사용: python -I -X utf8 ue_exec.py <파일.py>   또는   python -I -X utf8 ue_exec.py -c "print(1)"
# python은 엔진 것: D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
# -X utf8이 없으면 한글 출력이 깨짐 (-I 모드는 PYTHONIOENCODING을 무시함)
import sys
import time

sys.path.insert(0, r"D:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution as re_mod


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "-c":
        code = sys.argv[2]
    else:
        # 파일은 경로를 그대로 보냄 (내용을 보내면 ".py"가 들어간 줄을 파일 경로로 착각함)
        import os
        code = os.path.abspath(sys.argv[1]).replace("\\", "/")

    remote = re_mod.RemoteExecution()
    remote.start()
    try:
        # 에디터 찾기 (최대 5초)
        nodes = []
        for _ in range(50):
            # TwinSwift 에디터만 고름 (1차 에디터가 같이 켜져 있어도 헷갈리지 않게)
            nodes = [n for n in remote.remote_nodes if n.get("project_name") == "TwinSwift"]
            if nodes:
                break
            time.sleep(0.1)
        if not nodes:
            print("에디터를 찾지 못함 (에디터가 켜져 있는지, 원격 실행이 켜져 있는지 확인)")
            sys.exit(1)

        remote.open_command_connection(nodes[0]["node_id"])
        result = remote.run_command(code, unattended=True, exec_mode=re_mod.MODE_EXEC_FILE)
        for line in result.get("output", []):
            print(line.get("output", "").rstrip())
        if not result.get("success"):
            print("실패:", result.get("result"))
            sys.exit(2)
    finally:
        remote.stop()


main()
