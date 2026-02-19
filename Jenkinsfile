pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT = 'C:\\msys64'

    // Makefile がリポジトリ直下にある前提（別フォルダなら例: 'src' 等に変更）
    MAKE_WORKDIR = '.'

    // make 引数（必要なら例: '-j4 all'）
    MAKE_ARGS = ''

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    DIFF_FILE_LIST = 'diff_file_list.txt'
    KW_BUILD_SPEC  = 'kwinject.out'
  }

  stages {
    stage('Checkout') {
      steps { checkout scm }
    }

    stage('Create diff file list') {
      steps {
        bat """
          @echo off
          if exist "%DIFF_FILE_LIST%" del /f /q "%DIFF_FILE_LIST%"

          git rev-parse HEAD~1 >nul 2>nul
          if %ERRORLEVEL%==0 (
            git diff --name-only HEAD~1 HEAD > "%DIFF_FILE_LIST%"
          ) else (
            git ls-files > "%DIFF_FILE_LIST%"
          )

          for %%A in ("%DIFF_FILE_LIST%") do if %%~zA==0 (
            git ls-files > "%DIFF_FILE_LIST%"
          )

          echo [INFO] diff file list:
          type "%DIFF_FILE_LIST%"
        """
      }
    }

    stage('Resolve make path') {
      steps {
        script {
          def c1 = "${env.MSYS2_ROOT}\\mingw64\\bin\\mingw32-make.exe"
          def c2 = "${env.MSYS2_ROOT}\\usr\\bin\\make.exe"

          if (fileExists(c1))      { env.MAKE_EXE = c1 }
          else if (fileExists(c2)) { env.MAKE_EXE = c2 }
          else {
            error("make が見つかりません: ${c1} / ${c2}")
          }
        }

        bat """
          @echo off
          echo [INFO] Using MAKE_EXE=%MAKE_EXE%
          "%MAKE_EXE%" --version
          echo [INFO] MAKE_WORKDIR=%MAKE_WORKDIR%
          if not exist "%WORKSPACE%\\%MAKE_WORKDIR%" (
            echo [ERROR] workdir not found: %WORKSPACE%\\%MAKE_WORKDIR%
            dir "%WORKSPACE%"
            exit /b 2
          )
        """
      }
    }

    stage('Klocwork Analysis') {
      steps {
        // MSYS2 の sh 等を呼ぶ Makefile もあるので PATH も足す（害はほぼない）
        withEnv([
          "PATH+MSYS2_USR=${env.MSYS2_ROOT}\\usr\\bin",
          "PATH+MSYS2_MINGW64=${env.MSYS2_ROOT}\\mingw64\\bin"
        ]) {
          klocworkWrapper(
            installConfig: '-- なし --',
            ltoken: "${env.KW_LTOKEN}",
            serverConfig: "${env.KW_SERVER_CONFIG}",
            serverProject: "${env.KW_SERVER_PROJECT}"
          ) {
            klocworkBuildSpecGeneration([
              additionalOpts: '',
              buildCommand  : "\"${env.MAKE_EXE}\" ${env.MAKE_ARGS}",
              ignoreErrors  : false,
              output        : "${env.KW_BUILD_SPEC}",
              tool          : 'kwinject',
              // ★ここが肝：空やnullにさせない
              workDir       : "${env.MAKE_WORKDIR}"
            ])

            klocworkIncremental([
              additionalOpts: '',
              buildSpec     : "${env.KW_BUILD_SPEC}",
              cleanupProject: false,
              differentialAnalysisConfig: [
                diffFileList      : "${env.DIFF_FILE_LIST}",
                diffType          : 'git',
                gitPreviousCommit : ''
              ],
              incrementalAnalysis: false,
              projectDir        : '',
              reportFile        : ''
            ])
          }
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
    }
  }
}
