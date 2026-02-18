pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // パスは「クォート無し」で持って、使うときに必ず " " で囲う
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // ★追加：kwinject.out の compile dir（=vs2022）の1個上。相対 ..\\revisions をここに置く
    SAMPLE_BASE   = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes'
    SAMPLE_VS2022 = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022'

    // ここは Jenkins の「Klocwork Server Config」の名前に合わせる
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'

    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm

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
          bat 'if exist diff_file_list.txt del /f /q diff_file_list.txt'
          // ★追加：rawも消す
          bat 'if exist diff_file_list_raw.txt del /f /q diff_file_list_raw.txt'

          // 差分ファイル一覧を作成（raw）
          bat '''
            @echo on
            git rev-parse --verify HEAD >nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)
            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list_raw.txt
            ) || (
              type nul > diff_file_list_raw.txt
            )
          '''

          // ★追加：diffを build spec 形式（..\\revisions\\... / \\区切り）に変換して diff_file_list.txt を作る
          // - revisions/ 以外は捨てる（必要なら後で拡張）
          bat '''
            @echo on
            type nul > diff_file_list.txt
            for /f "usebackq delims=" %%F in ("diff_file_list_raw.txt") do (
              set "p=%%F"
              setlocal enabledelayedexpansion
              set "p=!p:/=\\!"
              echo !p! | findstr /b /i "revisions\\">nul
              if !errorlevel! == 0 (
                echo ..\\!p!>> diff_file_list.txt
              )
              endlocal
            )
          '''

          // ★追加：workspace の revisions を、サンプル側（SAMPLE_BASE\\revisions）へ同期
          // kwinject.out は compile dir=...\\vs2022 なので、..\\revisions は SAMPLE_BASE\\revisions を指す
          bat """
            @echo on
            if not exist "${env.SAMPLE_BASE}\\revisions" mkdir "${env.SAMPLE_BASE}\\revisions"
            robocopy "%WORKSPACE%\\revisions" "${env.SAMPLE_BASE}\\revisions" /MIR /NFL /NDL /NJH /NJS
            if %ERRORLEVEL% GEQ 8 exit /b 1
          """

          // diff_file_list の中身を表示（確認用）
          bat '''
            @echo on
            echo ===== diff_file_list_raw.txt =====
            if exist diff_file_list_raw.txt type diff_file_list_raw.txt
            for %%A in (diff_file_list_raw.txt) do @echo [INFO] diff_file_list_raw.txt bytes=%%~zA
            echo ===== diff_file_list.txt (converted) =====
            if exist diff_file_list.txt type diff_file_list.txt
            for %%A in (diff_file_list.txt) do @echo [INFO] diff_file_list.txt bytes=%%~zA
            echo ==========================================
          '''

          // 1) kwinject（workDir は空のままでもOK。compile dir は sln の場所に依存）
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

          // 2) 差分解析（Diff Analysis）
          // ★変更点：diffFileList は「converted（..\\revisions\\...）」の diff_file_list.txt を使う
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
        }
      }
    }
  }

  post {
    always {
      // ★rawも保存して比較できるようにする
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list*.txt,kwtables/**', onlyIfSuccessful: false
    }
  }
}
