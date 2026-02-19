pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    // MSYS2の想定ルート（違うならここだけ直す）
    MSYS2_ROOT = 'C:\\msys64'

    // Makefileの場所（リポジトリ直下なら空）
    MAKE_WORKDIR = ''

    // make 引数（例: -j4 / all / clean など）
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
          // 候補（まずmingw32-make優先、無ければusr/bin/make）
          def c1 = "${env.MSYS2_ROOT}\\mingw64\\bin\\mingw32-make.exe"
          def c2 = "${env.MSYS2_ROOT}\\usr\\bin\\make.exe"

          if (fileExists(c1)) {
            env.MAKE_EXE = c1
          } else if (fileExists(c2)) {
            env.MAKE_EXE = c2
          } else {
            error("""make が見つかりません。
想定パス:
- ${c1}
- ${c2}

対処:
- MSYS2 を ${env.MSYS2_ROOT} にインストール
- MSYS2で pacman -S make / pacman -S mingw-w64-x86_64-gcc を実行
- もしくは MAKE_EXE を実在パスに修正
""")
          }
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
