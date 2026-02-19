pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT = 'C:\\msys64'
    MAKE_WORKDIR = '.'
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
          if (fileExists(c1)) env.MAKE_EXE = c1
          else if (fileExists(c2)) env.MAKE_EXE = c2
          else error("make が見つかりません: ${c1} / ${c2}")
        }

        bat """
          @echo off
          echo [INFO] Using MAKE_EXE=%MAKE_EXE%
          "%MAKE_EXE%" --version
        """
      }
    }

    stage('Klocwork Analysis') {
      steps {
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
            // BuildSpec生成（kwinjectを手動実行）
            bat """
              @echo off
              cd /d "%WORKSPACE%\\%MAKE_WORKDIR%"

              echo [INFO] kwinject version
              kwinject --version

              if exist "%WORKSPACE%\\%KW_BUILD_SPEC%" del /f /q "%WORKSPACE%\\%KW_BUILD_SPEC%"

              echo [INFO] generating build spec: %KW_BUILD_SPEC%
              echo [INFO] run: kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
              kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
            """

            // ★差分解析：ご指定の内容を反映
            // 注意：初回ビルド等で GIT_PREVIOUS_SUCCESSFUL_COMMIT が空だと差分解析にならない場合あり
            klocworkIncremental([
              additionalOpts: '',
              buildSpec     : "${env.KW_BUILD_SPEC}",
              cleanupProject: false,
              differentialAnalysisConfig: [
                diffFileList     : "${env.DIFF_FILE_LIST}",
                diffType         : 'git',
                gitPreviousCommit: "${env.GIT_PREVIOUS_SUCCESSFUL_COMMIT}"
              ],
              incrementalAnalysis: true,
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
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
    }
  }
}
