pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // パスは「クォート無し」で持って、使うときに必ず " " で囲う
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // ここは Jenkins の「Klocwork Server Config」の名前に合わせる
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'

    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'

    // kwciagent 実行時のカレントディレクトリを固定する（..\revisions\... を %WORKSPACE%\revisions\... に解決させる）
    KW_CWD_SUBDIR = '_kwcwd'
  }

  stages {
    stage('Checkout') {
      steps {
        // まず通常 checkout
        checkout scm

        // shallow clone 等で HEAD~1 が無い問題を潰す（失敗しても続行）
        bat """
          @echo on
          git rev-parse --is-inside-work-tree
          git fetch --tags --prune --progress
          git fetch --unshallow || exit /b 0
        """
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
          bat "if exist kwinject.out del /f /q kwinject.out"
          bat "if exist kwtables rmdir /s /q kwtables"
          bat "if exist diff_file_list_raw.txt del /f /q diff_file_list_raw.txt"
          bat "if exist diff_file_list.txt del /f /q diff_file_list.txt"

          // 差分ファイル一覧を作成（repoルート基準）
          // HEAD~1 が無い場合は空ファイルにする
          bat """
            @echo on
            git rev-parse --verify HEAD >nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)
            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list_raw.txt
            ) || (
              type nul > diff_file_list_raw.txt
            )
          """

          // raw の中身をコンソールに表示（確認用）
          bat """
            @echo on
            echo ===== diff_file_list_raw.txt =====
            if exist diff_file_list_raw.txt type diff_file_list_raw.txt
            echo ================================
          """

          // diff_file_list.txt を kwinject.out の記録形式（..\revisions\...）に合わせて作る
          // - / -> \ 変換
          // - revisions\... は ..\revisions\... に寄せる
          // - 解析対象外（Jenkinsfile等）を混ぜないため拡張子でフィルタ
          bat """
            @echo on
            setlocal EnableDelayedExpansion
            type nul > diff_file_list.txt

            for /F "usebackq delims=" %%F in ("diff_file_list_raw.txt") do (
              set "p=%%F"
              if not "!p!"=="" (
                set "p=!p:/=\\!"

                rem --- 拡張子フィルタ（必要なら増やしてOK）---
                echo !p! | findstr /R /I "\\.c$ \\.cc$ \\.cpp$ \\.cxx$ \\.h$ \\.hpp$ \\.hh$" >nul
                if !errorlevel! EQU 0 (
                  if /I "!p:~0,10!"=="revisions\\" (
                    echo ..\\!p!>> diff_file_list.txt
                  ) else (
                    echo !p!>> diff_file_list.txt
                  )
                ) else (
                  echo [SKIP] !p!
                )
              )
            )

            endlocal
          """

          // converted の中身を表示
          bat """
            @echo on
            echo ===== diff_file_list.txt (for kwciagent) =====
            if exist diff_file_list.txt type diff_file_list.txt
            for %%A in (diff_file_list.txt) do @echo [INFO] diff_file_list.txt bytes=%%~zA
            echo ============================================
          """

          // kwciagent 実行用のカレントディレクトリを用意（%WORKSPACE%\\_kwcwd）
          bat """
            @echo on
            if not exist "%WORKSPACE%\\${env.KW_CWD_SUBDIR}" mkdir "%WORKSPACE%\\${env.KW_CWD_SUBDIR}"
          """

          // 1) kwinject（必ず二重引用符）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"${env.MSBUILD}\" \"${env.SLN}\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat "if not exist kwinject.out exit /b 1"
          bat "for %%A in (kwinject.out) do if %%~zA==0 exit /b 1"

          // 2) 差分解析（Diff Analysis）
          // diff_file_list.txt の中身が ..\\revisions\\... なので、kwciagent の cwd を %WORKSPACE%\\_kwcwd に固定する
          klocworkIncremental([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList: 'diff_file_list.txt',
              diffType: 'git',
              gitPreviousCommit: 'HEAD~1'
            ],
            incrementalAnalysis: true,
            projectDir: '',
            reportFile: ''
          ])

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
