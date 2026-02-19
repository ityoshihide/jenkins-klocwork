pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // Makefile を使って build spec を生成する
    // ※ Windows エージェントで `make` が実行できる必要があります（例: MSYS2 / MinGW / Cygwin など）
    MAKE = 'make'

    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'
    KW_LTOKEN        = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {

    stage('Checkout') {
      steps {
        checkout scm
        bat '''
          git rev-parse --is-inside-work-tree
          git fetch --tags --prune --progress
          git fetch --unshallow || exit /b 0
        '''
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {

        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_PROJECT}"
        ) {

          // Cleanup
          bat '''
            if exist kwinject.out del /f /q kwinject.out
            if exist kwtables rmdir /s /q kwtables
            if exist diff_file_list_raw.txt del /f /q diff_file_list_raw.txt
            if exist diff_file_list.txt del /f /q diff_file_list.txt
          '''

          // Makefile の存在チェック（GitHub上のコードを対象にする）
          bat '''
            if not exist "Makefile" (
              echo [ERROR] Makefile not found in WORKSPACE: "%WORKSPACE%"
              exit /b 1
            )
            where %MAKE% >nul 2>nul || (
              echo [ERROR] make command not found. Please install make (e.g., MSYS2/MinGW/Cygwin) and add it to PATH.
              exit /b 1
            )
          '''

          // diff 抽出
          bat '''
            git rev-parse --verify HEAD >nul 2>nul || exit /b 1

            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list_raw.txt
            ) || (
              type nul > diff_file_list_raw.txt
            )
          '''

          // C/C++ のみ抽出（goto無し・ラベル無し）
          bat '''
            setlocal EnableDelayedExpansion
            type nul > diff_file_list.txt

            for /F "usebackq delims=" %%F in ("diff_file_list_raw.txt") do (
              set "p=%%F"
              if not "!p!"=="" (
                set "ext=%%~xF"

                if /I "!ext!"==".c" (
                  set "p=!p:/=\\!"
                  echo !p!>> diff_file_list.txt
                )
                if /I "!ext!"==".cc" (
                  set "p=!p:/=\\!"
                  echo !p!>> diff_file_list.txt
                )
                if /I "!ext!"==".cpp" (
                  set "p=!p:/=\\!"
                  echo !p!>> diff_file_list.txt
                )
                if /I "!ext!"==".cxx" (
                  set "p=!p:/=\\!"
                  echo !p!>> diff_file_list.txt
                )
              )
            )

            endlocal
          '''

          bat '''
            echo ===== diff_file_list_raw.txt =====
            if exist diff_file_list_raw.txt type diff_file_list_raw.txt
            echo ===== diff_file_list.txt =====
            if exist diff_file_list.txt type diff_file_list.txt
          '''

          // 差分がC/C++ 0件なら解析スキップ
          bat '''
            for %%A in (diff_file_list.txt) do set "SZ=%%~zA"
            if not defined SZ set "SZ=0"
            if %SZ%==0 (
              echo [INFO] C/C++ diff is empty. Skip kwciagent run.
              exit /b 0
            )
          '''

          // ビルドスペック生成（GitHubのWORKSPACE上の Makefile を使う）
          // -B: 常に再ビルド（compile を確実に発生させて build info を作る）
          klocworkBuildSpecGeneration([
            buildCommand: "\"${env.MAKE}\" -B",
            output: 'kwinject.out',
            tool: 'kwinject'
          ])

          // ガード
          bat '''
            if not exist kwinject.out exit /b 1
            for %%A in (kwinject.out) do if %%~zA==0 exit /b 1
          '''

          // kwciagent run（workspace直下で実行）
          bat '''
            kwciagent set --project-dir "%WORKSPACE%\\.kwlp" klocwork.host=192.168.137.1 klocwork.port=2540 klocwork.project=jenkins_demo

            kwciagent run ^
              --project-dir "%WORKSPACE%\\.kwlp" ^
              --license-host 192.168.137.1 ^
              --license-port 27000 ^
              -Y -L ^
              --build-spec "%WORKSPACE%\\kwinject.out" ^
              @"%WORKSPACE%\\diff_file_list.txt"
          '''

          // 必要なら同期
          // klocworkIssueSync([...])
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list_raw.txt,diff_file_list.txt,kwtables/**', onlyIfSuccessful: false
    }
  }
}
