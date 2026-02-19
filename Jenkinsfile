// Jenkinsfile（make だけに絞った最小構成）
// ※MSYS2 を想定：C:\msys64\mingw64\bin\mingw32-make.exe

pipeline {
  agent any

  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    // ===== make のフルパス（ここだけ合わせればOK）=====
    MAKE_EXE = 'C:\\msys64\\mingw64\\bin\\mingw32-make.exe'

    // Makefile のあるディレクトリ（リポジトリ直下なら空）
    MAKE_WORKDIR = ''

    // make に渡す引数（例：-j4 / target など）
    MAKE_ARGS = ''
    // ===================================================

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    DIFF_FILE_LIST = 'diff_file_list.txt'
    KW_BUILD_SPEC  = 'kwinject.out'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
      }
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

    stage('Verify make') {
      steps {
        bat """
          @echo off
          if not exist "%MAKE_EXE%" (
            echo [ERROR] make not found: %MAKE_EXE%
            exit /b 2
          )
          "%MAKE_EXE%" --version
        """
      }
    }

    stage('Klocwork Analysis') {
      steps {
        // Jenkinsサービス環境で PATH が通ってなくても動くように、
        // make はフルパス指定で呼ぶ（sh/gcc等が必要なら PATH 追加を検討）
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

  post {
    always {
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
    }
  }
}
