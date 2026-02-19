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
  }

  stages {
    stage('Checkout') {
      steps {
        // まず通常 checkout
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
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'
          bat 'if exist diff_file_list_raw.txt del /f /q diff_file_list_raw.txt'
          bat 'if exist diff_file_list.txt del /f /q diff_file_list.txt'

          // 1) 差分ファイル一覧（生）を作成
          // HEAD~1 が無い場合は空ファイル
          bat '''
            @echo on
            git rev-parse --verify HEAD >nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)

            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list_raw.txt
            ) || (
              type nul > diff_file_list_raw.txt
            )
          '''

          // 2) 生リストを「workspace基準の絶対パス」に変換して diff_file_list.txt を作成
          //    - %CD% は C:\\ProgramData\\Jenkins\\.jenkins\\workspace\\klocwork-pipeline
          //    - / を \\ に変換
          //    - 空行は除外
          bat '''
            @echo on
            echo ===== diff_file_list_raw.txt =====
            if exist diff_file_list_raw.txt type diff_file_list_raw.txt
            echo ================================

            setlocal EnableDelayedExpansion
            type nul > diff_file_list.txt

            for /f "usebackq delims=" %%F in ("diff_file_list_raw.txt") do (
              set "p=%%F"
              if not "!p!"=="" (
                set "p=!p:/=\\!"
                echo %CD%\\!p!>> diff_file_list.txt
              )
            )

            endlocal

            echo ===== diff_file_list.txt (ABS) =====
            if exist diff_file_list.txt type diff_file_list.txt
            for %%A in (diff_file_list.txt) do @echo [INFO] diff_file_list.txt bytes=%%~zA
            echo ===================================
          '''

          // 3) 変換したパスが実在するかチェック（NGなら即失敗）
          bat '''
            @echo on
            echo ===== verify diff paths exist =====
            for /f "usebackq delims=" %%F in ("diff_file_list.txt") do (
              if exist "%%F" (
                echo [OK] %%F
              ) else (
                echo [NG] %%F
                exit /b 1
              )
            )
            echo ==================================
          '''

          // 4) kwinject（"C:\\Program" is not executable 対策：必ず二重引用符）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"${env.MSBUILD}\" \"${env.SLN}\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat 'if not exist kwinject.out exit /b 1'
          bat 'for %%A in (kwinject.out) do if %%~zA==0 exit /b 1'

          // 5) 差分解析（Diff Analysis）
          // 重要：diff_file_list.txt は「絶対パス」で作っているので、kwciagent が確実に解決できる
          // ※プラグイン側が git diff で diff_file_list.txt を上書きする挙動が残る場合は、
          //   differentialAnalysisConfig の指定方法を見直すか、kwciagent を bat で自前実行に切替える
          klocworkIncremental([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList: 'diff_file_list.txt',
              diffType: 'file'
            ],
            incrementalAnalysis: true,
            projectDir: '',
            reportFile: ''
          ])

          // 6) Jenkinsへ指摘を同期（Dashboard表示を安定させる）
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
