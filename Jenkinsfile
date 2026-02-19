pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // パスは「クォート無し」で持って、使うときに必ず " " で囲う
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // Jenkins の「Klocwork Server Config」の名前に合わせる
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'

    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'

    // kwciagent run のカレントを固定するためのサブフォルダ
    KW_CWD_SUBDIR = '_kwcwd'

    // kwciagent の作業ディレクトリ（workspace配下）
    KWLP_DIR = '.kwlp'
    KWPS_DIR = '.kwps'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm

        // shallow clone 等で HEAD~1 が無い問題を潰す（失敗しても続行）
        bat '''
          @echo on
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

          // 毎回クリーン
          bat '''
            @echo on
            if exist kwinject.out del /f /q kwinject.out
            if exist kwtables rmdir /s /q kwtables
            if exist diff_file_list_raw.txt del /f /q diff_file_list_raw.txt
            if exist diff_file_list.txt del /f /q diff_file_list.txt
          '''

          // 差分ファイル一覧を作成（repoルート基準）
          bat '''
            @echo on
            git rev-parse --verify HEAD >nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)
            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list_raw.txt
            ) || (
              type nul > diff_file_list_raw.txt
            )
          '''

          // raw の中身を表示
          bat '''
            @echo on
            echo ===== diff_file_list_raw.txt =====
            if exist diff_file_list_raw.txt type diff_file_list_raw.txt
            echo ================================
          '''

          // diff_file_list.txt を kwinject.out の表記（..\revisions\...）に寄せて作る
          // - / -> バックスラッシュ に変換
          // - revisions\... は ..\revisions\... に寄せる
          // - 拡張子でフィルタ（正規表現は使わない：$ を回避）
          bat '''
            @echo on
            setlocal EnableDelayedExpansion
            type nul > diff_file_list.txt

            for /F "usebackq delims=" %%F in ("diff_file_list_raw.txt") do (
              set "p=%%F"
              if not "!p!"=="" (
                rem / をバックスラッシュに変換
                set "p=!p:/=\\!"

                rem 拡張子でフィルタ（必要に応じて追加OK）
                set "ext=%%~xF"
                if /I "!ext!"==".c"   goto _ACCEPT
                if /I "!ext!"==".cc"  goto _ACCEPT
                if /I "!ext!"==".cpp" goto _ACCEPT
                if /I "!ext!"==".cxx" goto _ACCEPT
                if /I "!ext!"==".h"   goto _ACCEPT
                if /I "!ext!"==".hh"  goto _ACCEPT
                if /I "!ext!"==".hpp" goto _ACCEPT
                echo [SKIP] !p!
                goto _NEXT

                :_ACCEPT
                if /I "!p:~0,10!"=="revisions\\" (
                  echo ..\\!p!>> diff_file_list.txt
                ) else (
                  echo !p!>> diff_file_list.txt
                )

                :_NEXT
              )
            )

            endlocal
          '''

          // converted の中身を表示
          bat '''
            @echo on
            echo ===== diff_file_list.txt (for kwciagent) =====
            if exist diff_file_list.txt type diff_file_list.txt
            for %%A in (diff_file_list.txt) do @echo [INFO] diff_file_list.txt bytes=%%~zA
            echo ============================================
          '''

          // kwciagent 実行用のカレントディレクトリを用意（%WORKSPACE%\\_kwcwd）
          bat '''
            @echo on
            if not exist "%WORKSPACE%\\%KW_CWD_SUBDIR%" mkdir "%WORKSPACE%\\%KW_CWD_SUBDIR%"
          '''

          // 1) kwinject（ビルドスペック生成）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"${env.MSBUILD}\" \"${env.SLN}\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat '''
            @echo on
            if not exist kwinject.out exit /b 1
            for %%A in (kwinject.out) do if %%~zA==0 exit /b 1
            echo [INFO] kwinject.out path = %WORKSPACE%\\kwinject.out
          '''

          // 2) kwciagent を "bat で直叩き"（cd を確実に効かせる）
          bat '''
            @echo on

            rem --- kwciagent プロジェクト作業領域を作成 ---
            if not exist "%WORKSPACE%\\%KWLP_DIR%" mkdir "%WORKSPACE%\\%KWLP_DIR%"
            if not exist "%WORKSPACE%\\%KWPS_DIR%" mkdir "%WORKSPACE%\\%KWPS_DIR%"

            rem --- create ---
            kwciagent create --url http://192.168.137.1:2540/%KW_PROJECT% --project-dir "%WORKSPACE%\\%KWLP_DIR%" --settings-dir "%WORKSPACE%\\%KWPS_DIR%" --build-spec "%WORKSPACE%\\kwinject.out"
            if errorlevel 1 exit /b 1

            rem --- ここが重要：cwd を %WORKSPACE%\\_kwcwd に固定 ---
            cd /d "%WORKSPACE%\\%KW_CWD_SUBDIR%"

            rem --- diff のパスが存在するか確認（..\\revisions\\... が %WORKSPACE%\\revisions\\... に解決される想定）---
            echo ===== verify diff paths exist (from %CD%) =====
            for /F "usebackq delims=" %%F in ("%WORKSPACE%\\diff_file_list.txt") do (
              if exist "%%F" (echo [OK] %%F) else (echo [NG] %%F & exit /b 1)
            )
            echo =============================================

            rem --- run ---
            kwciagent run --project-dir "%WORKSPACE%\\%KWLP_DIR%" --license-host 192.168.137.1 --license-port 27000 -Y -L --build-spec "%WORKSPACE%\\kwinject.out" @"%WORKSPACE%\\diff_file_list.txt"
            if errorlevel 1 exit /b 1

            rem --- list（任意）---
            kwciagent list --project-dir "%WORKSPACE%\\%KWLP_DIR%" --license-host 192.168.137.1 --license-port 27000 -F xml @"%WORKSPACE%\\diff_file_list.txt" || exit /b 0
          '''

          // 3) Jenkins に結果連携（XSync）
          klocworkIssueSync([
            additionalOpts: '',
            dryRun: false,
            lastSync: '03-00-0000 00:00:00',
            projectRegexp: '',
            statusAnalyze: true,
            statusDefer: true,
            statusFilter: true,
            statusFix: true,
            statusFixInLaterRelease: false,
            statusFixInNextRelease: true,
            statusIgnore: true,
            statusNotAProblem: true
          ])
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
