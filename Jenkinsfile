// Jenkinsfile
// 前提：Windows エージェントで実行（bat が使える）
// GitHub Webhook などで「コミット時に起動」させる想定

pipeline {
  agent any

  triggers {
    // GitHub の push で起動（GitHub plugin / Webhook 設定が前提）
    githubPush()
    // もし Webhook が使えない場合は pollSCM を使う
    // pollSCM('H/5 * * * *')
  }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    // Klocwork プラグインの引数として渡す値
    KW_LTOKEN        = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_SERVER_PROJECT= 'jenkins_demo'

    // 差分ファイル一覧
    DIFF_FILE_LIST   = 'diff_file_list.txt'

    // kwinject 出力
    KW_BUILD_SPEC    = 'kwinject.out'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm

        // 差分算出に必要なので shallow clone を避けたい（環境によっては fetch を追加）
        bat '''
          git --version
          git rev-parse --is-inside-work-tree
        '''
      }
    }

    stage('Create diff file list') {
      steps {
        bat """
          @echo off
          setlocal enabledelayedexpansion

          rem 直前コミット（前回ビルド）を取得できないケースに備えて安全に作る
          if exist "%DIFF_FILE_LIST%" del /f /q "%DIFF_FILE_LIST%"

          rem 前回ビルドのコミットが取れるか確認
          git rev-parse HEAD~1 >nul 2>nul
          if %ERRORLEVEL%==0 (
            echo [INFO] diff: HEAD~1..HEAD
            git diff --name-only HEAD~1 HEAD > "%DIFF_FILE_LIST%"
          ) else (
            echo [WARN] HEAD~1 が無い（初回ビルド等）ため、全ファイルを対象にします
            git ls-files > "%DIFF_FILE_LIST%"
          )

          rem 空だと困るので保険（サブモジュール等で空になるケース）
          for %%A in ("%DIFF_FILE_LIST%") do if %%~zA==0 (
            echo [WARN] diff list is empty. fallback to all files.
            git ls-files > "%DIFF_FILE_LIST%"
          )

          echo [INFO] diff file list:
          type "%DIFF_FILE_LIST%"
        """
      }
    }

    stage('Klocwork Analysis') {
      steps {
        // ここで serverConfig / serverProject / ltoken をまとめて指定
        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_SERVER_PROJECT}"
        ) {
          // 1) BuildSpec 生成（kwinject）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand  : 'make',
            ignoreErrors  : false,
            output        : "${env.KW_BUILD_SPEC}",
            tool          : 'kwinject',
            workDir       : ''
          ])

          // 2) Incremental（差分ファイルリストを使う）
          klocworkIncremental([
            additionalOpts: '',
            buildSpec     : "${env.KW_BUILD_SPEC}",
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList      : "${env.DIFF_FILE_LIST}",
              diffType          : 'git',
              gitPreviousCommit : ''   // 空でOK（diffFileList を主に使う）
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
