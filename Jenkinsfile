// Jenkinsfile (Klocwork公式プラグイン版)

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

        bat """
          @echo off
          echo [INFO] Using MAKE_EXE=%MAKE_EXE%
          "%MAKE_EXE%" --version
        """
      }
    }

    stage('Decide previous commit') {
      steps {
        script {
          def prev = env.GIT_PREVIOUS_SUCCESSFUL_COMMIT
          if (!prev?.trim()) prev = env.GIT_PREVIOUS_COMMIT

          if (!prev?.trim()) {
            def rc = bat(returnStatus: true, script: 'git rev-parse HEAD~1 > .prev_commit 2>nul')
            if (rc == 0) {
              prev = readFile('.prev_commit').trim()
            }
          }

          if (prev?.trim()) {
            env.KW_PREV_COMMIT = prev.trim()
            echo "Using KW_PREV_COMMIT=${env.KW_PREV_COMMIT}"
          } else {
            env.KW_PREV_COMMIT = ''
            echo "KW_PREV_COMMIT not available -> fallback to full analysis"
          }
        }
      }
    }

    stage('Create diff file list') {
      steps {
        script {
          if (!env.KW_PREV_COMMIT?.trim()) {
            bat """
              @echo off
              git ls-files > "%DIFF_FILE_LIST%"
              echo [INFO] diff file list (fallback: all files):
              type "%DIFF_FILE_LIST%"
            """
          } else {
            bat """
              @echo off
              git diff --name-only %KW_PREV_COMMIT% HEAD > "%DIFF_FILE_LIST%"
              for %%A in ("%DIFF_FILE_LIST%") do if %%~zA==0 (
                git ls-files > "%DIFF_FILE_LIST%"
              )
              echo [INFO] diff file list (%KW_PREV_COMMIT%..HEAD):
              type "%DIFF_FILE_LIST%"
            """
          }
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
            bat """
              @echo off
              cd /d "%WORKSPACE%\\%MAKE_WORKDIR%"
              echo [INFO] generating build spec: %KW_BUILD_SPEC%
              kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
            """

            // 解析の実行
            script {
              if (env.KW_PREV_COMMIT?.trim()) {
                // 差分解析
                klocworkIncremental(
                  buildSpec: "${env.KW_BUILD_SPEC}",
                  differentialAnalysisConfig: [
                    diffFileList: "${env.DIFF_FILE_LIST}",
                    diffType: 'git',
                    gitPreviousCommit: "${env.KW_PREV_COMMIT}"
                  ],
                  incrementalAnalysis: true
                )
              } else {
                // フル解析
                klocworkIncremental(
                  buildSpec: "${env.KW_BUILD_SPEC}",
                  incrementalAnalysis: false
                )
              }
            }
          }
        }
      }
    }

    // ★★★ ここからが変更箇所 ★★★
    // Klocworkプラグインの標準機能を使って結果を発行するステージ
    stage('Publish Klocwork Results') {
      steps {
        echo "Publishing Klocwork results to Jenkins dashboard..."
        
        // このステップが、ビルド結果ページに「Klocwork解析結果」のグラフと表を追加します。
        // 解析は既に終わっているので、ここでは結果をJenkinsに登録するだけです。
        klocworkIntegration(
            // Klocworkサーバーから直接結果を取得するため、レポートファイルの指定は不要
            // reportFile: '' 
        )
      }
    }

    // Klocworkプラグインの標準機能で品質ゲートを評価するステージ
    stage('Klocwork Quality Gateway') {
        steps {
            echo "Checking Klocwork quality gateway..."
            
            // このステップが、検出された問題の数に基づいてビルドステータスを変更します。
            klocworkQualityGateway(
                // 例：'Critical' な問題が1件でもあればビルドを 'UNSTABLE' にする
                unstableConditions: [
                    [severity: 'Critical', threshold: '0']
                ],
                // 例：'Critical' な問題が10件以上あればビルドを 'FAILURE' にする
                failedConditions: [
                    [severity: 'Critical', threshold: '9']
                ]
            )
        }
    }
  }

  post {
    always {
      // 不要になったSARIF関連のアーティファクトを削除
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
    }
  }
}
