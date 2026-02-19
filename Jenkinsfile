// Jenkinsfile (Klocwork公式プラグインによる結果表示のみ)

pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT    = 'C:\\msys64'
    MAKE_WORKDIR  = '.'
    MAKE_ARGS     = ''

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    DIFF_FILE_LIST = 'diff_file_list.txt'
    KW_BUILD_SPEC  = 'kwinject.out'
  }

  stages {
    // ... (CheckoutからKlocwork Build & Analysisまでのステージは変更なし) ...

    stage('Checkout') {
      steps { checkout scm }
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
        bat 'echo [INFO] Using MAKE_EXE=%MAKE_EXE% && "%MAKE_EXE%" --version'
      }
    }

    stage('Decide previous commit') {
      steps {
        script {
          def prev = env.GIT_PREVIOUS_SUCCESSFUL_COMMIT ?: env.GIT_PREVIOUS_COMMIT
          if (!prev?.trim()) {
            def rc = bat(returnStatus: true, script: 'git rev-parse HEAD~1 > .prev_commit 2>nul')
            if (rc == 0) { prev = readFile('.prev_commit').trim() }
          }
          env.KW_PREV_COMMIT = prev?.trim() ?: ''
          echo "[INFO] Using KW_PREV_COMMIT=${env.KW_PREV_COMMIT ?: 'none (full analysis)'}"
        }
      }
    }

    stage('Create diff file list') {
      steps {
        script {
          if (!env.KW_PREV_COMMIT) {
            bat 'git ls-files > "%DIFF_FILE_LIST%"'
          } else {
            bat '''
              git diff --name-only %KW_PREV_COMMIT% HEAD > "%DIFF_FILE_LIST%"
              for %%A in ("%DIFF_FILE_LIST%") do if %%~zA==0 (git ls-files > "%DIFF_FILE_LIST%")
            '''
          }
          bat 'echo [INFO] diff file list: && type "%DIFF_FILE_LIST%"'
        }
      }
    }

    stage('Klocwork Build & Analysis') {
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
            // BuildSpec生成
            bat 'cd /d "%WORKSPACE%\\%MAKE_WORKDIR%" && kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%'

            // 解析の実行
            script {
              if (env.KW_PREV_COMMIT) {
                klocworkIncremental(buildSpec: env.KW_BUILD_SPEC, differentialAnalysisConfig: [diffFileList: env.DIFF_FILE_LIST, diffType: 'git', gitPreviousCommit: env.KW_PREV_COMMIT], incrementalAnalysis: true)
              } else {
                klocworkIncremental(buildSpec: env.KW_BUILD_SPEC, incrementalAnalysis: false)
              }
            }
          }
        }
      }
    }

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ これがKlocworkの結果をJenkins UIに表示させるためのステージです ★
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    stage('Publish Klocwork Results') {
      steps {
        echo "Publishing Klocwork results to Jenkins dashboard..."
        
        // この1ステップだけで、ビルド結果ページにグラフや表が追加されます。
        // 複雑なSARIF変換やWarnings NGプラグインは不要です。
        klocworkIntegration()
      }
    }
  }

  post {
    always {
      // 解析に使った中間ファイルをアーティファクトとして保存
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
    }
  }
}
