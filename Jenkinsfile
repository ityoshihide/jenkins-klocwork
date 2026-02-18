pipeline {
  agent any

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    // --- Klocwork / License ---
    KW_HOST       = '192.168.137.1'
    KW_PORT       = '2540'
    KW_SERVER_URL = "http://${KW_HOST}:${KW_PORT}"
    LICENSE_HOST  = '192.168.137.1'
    LICENSE_PORT  = '27000'
    KW_PROJECT    = 'jenkins_demo'

    // --- Visual Studio MSBuild ---
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'

    // --- Klocwork sample base (kwinject.out の compile dir と一致させる) ---
    SAMPLE_BASE  = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes'
    SAMPLE_VS2022 = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022'
    SAMPLE_SLN   = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // --- Klocwork CI working dir (workspace内) ---
    KWLP_DIR = "${WORKSPACE}\\.kwlp"
  }

  stages {

    stage('Checkout') {
      steps {
        checkout scm

        // 参考：git shallow 対策（ログにあったが、完全repoでは失敗しても無害化）
        bat '''
          @echo off
          git rev-parse --is-inside-work-tree || exit /b 1
          git fetch --tags --prune --progress
          git fetch --unshallow || exit /b 0
        '''
      }
    }

    stage('Klocwork Diff Analysis (Plan 1)') {
      steps {
        // Klocwork Jenkins Plugin の Wrapper を使う前提
        //（ログに [Pipeline] klocworkWrapper が出ているので同じ前提で記述）
        klocworkWrapper(
          klocworkServerUrl: "${env.KW_SERVER_URL}",
          licenseHost: "${env.LICENSE_HOST}",
          licensePort: "${env.LICENSE_PORT}"
        ) {
          bat """
            @echo off
            setlocal enabledelayedexpansion

            echo ============================================================
            echo [INFO] WORKSPACE     = %WORKSPACE%
            echo [INFO] SAMPLE_BASE   = ${env.SAMPLE_BASE}
            echo [INFO] SAMPLE_VS2022 = ${env.SAMPLE_VS2022}
            echo [INFO] SAMPLE_SLN    = ${env.SAMPLE_SLN}
            echo ============================================================

            rem --- clean ---
            if exist "%WORKSPACE%\\kwinject.out" del /f /q "%WORKSPACE%\\kwinject.out"
            if exist "%WORKSPACE%\\diff_file_list_raw.txt" del /f /q "%WORKSPACE%\\diff_file_list_raw.txt"
            if exist "%WORKSPACE%\\diff_file_list.txt" del /f /q "%WORKSPACE%\\diff_file_list.txt"

            rem --- 1) diff list generate (repo基準: revisions/...) ---
            git rev-parse --verify HEAD 1>nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)

            git rev-parse --verify HEAD~1 1>nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > "%WORKSPACE%\\diff_file_list_raw.txt"
            ) || (
              type nul > "%WORKSPACE%\\diff_file_list_raw.txt"
            )

            echo ===== diff_file_list_raw.txt =====
            type "%WORKSPACE%\\diff_file_list_raw.txt"
            echo ================================

            rem --- 2) diff list convert to build-spec style: ..\\revisions\\... (\\区切り, 先頭に..\\) ---
            for /f "usebackq delims=" %%F in ("%WORKSPACE%\\diff_file_list_raw.txt") do (
              set "p=%%F"
              set "p=!p:/=\\!"
              echo !p! | findstr /b /i "revisions\\">nul
              if !errorlevel! == 0 (
                echo ..\\!p!>> "%WORKSPACE%\\diff_file_list.txt"
              )
            )

            echo ===== diff_file_list.txt (converted) =====
            if exist "%WORKSPACE%\\diff_file_list.txt" (
              type "%WORKSPACE%\\diff_file_list.txt"
            ) else (
              echo [INFO] (empty)
              type nul > "%WORKSPACE%\\diff_file_list.txt"
            )
            echo ==========================================

            rem --- 3) sync repo's revisions -> sample demosthenes revisions (これが肝) ---
            if not exist "${env.SAMPLE_BASE}\\revisions" mkdir "${env.SAMPLE_BASE}\\revisions"
            echo [INFO] robocopy "%WORKSPACE%\\revisions" "${env.SAMPLE_BASE}\\revisions" /MIR ...
            robocopy "%WORKSPACE%\\revisions" "${env.SAMPLE_BASE}\\revisions" /MIR /NFL /NDL /NJH /NJS
            if %ERRORLEVEL% GEQ 8 (
              echo [ERROR] robocopy failed rc=%ERRORLEVEL%
              exit /b 1
            )

            rem --- 4) run kwinject using sample SLN (compile dir一致) ---
            pushd "${env.SAMPLE_VS2022}"

            "${env.MSBUILD}" "${env.SAMPLE_SLN}" /t:Rebuild
            if errorlevel 1 (
              echo [ERROR] MSBuild failed
              popd
              exit /b 1
            )

            kwinject --output "%WORKSPACE%\\kwinject.out" "${env.MSBUILD}" "${env.SAMPLE_SLN}" /t:Rebuild
            if errorlevel 1 (
              echo [ERROR] kwinject failed
              popd
              exit /b 1
            )

            popd

            rem --- 5) sanity check kwinject.out ---
            if not exist "%WORKSPACE%\\kwinject.out" (
              echo [ERROR] kwinject.out not found
              exit /b 1
            )
            for %%A in ("%WORKSPACE%\\kwinject.out") do (
              if %%~zA==0 (
                echo [ERROR] kwinject.out is empty
                exit /b 1
              )
              echo [INFO] kwinject.out bytes=%%~zA
            )

            rem --- 6) kwciagent set/run (diff-file list) ---
            if not exist "${env.KWLP_DIR}" mkdir "${env.KWLP_DIR}"

            kwciagent set --project-dir "${env.KWLP_DIR}" klocwork.host=${env.KW_HOST} klocwork.port=${env.KW_PORT} klocwork.project=${env.KW_PROJECT}
            if errorlevel 1 (
              echo [ERROR] kwciagent set failed
              exit /b 1
            )

            rem NOTE: diff_file_list.txt は build spec 形式に変換済み（..\\revisions\\...）
            kwciagent run --project-dir "${env.KWLP_DIR}" --license-host ${env.LICENSE_HOST} --license-port ${env.LICENSE_PORT} -Y -L --build-spec "%WORKSPACE%\\kwinject.out" @%WORKSPACE%\\diff_file_list.txt
            if errorlevel 1 (
              echo [ERROR] kwciagent run failed
              exit /b 1
            )

            endlocal
          """
        }
      }
    }

  } // stages

  post {
    always {
      // ログ・成果物の保存（必要に応じて追加）
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list*.txt,**/.kwlp/**', allowEmptyArchive: true
    }
  }
}
