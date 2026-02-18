pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // Klocwork Jenkins設定（Jenkins側の「Validateサーバー」設定名に合わせる）
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'

    // ltoken（Jenkins実行ユーザーから参照できること）
    KW_LTOKEN        = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {
        script {
          // Klocwork側の build 名を毎回一意に（例: jenkins-klocwork-pipeline-23）
          env.KW_BUILD_NAME = "jenkins-${env.JOB_NAME}-${env.BUILD_NUMBER}"
        }

        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_PROJECT}"
        ) {

          // クリーン
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'
          bat 'if exist diff_file_list.txt del /f /q diff_file_list.txt'

          // 0) 差分ファイルリスト生成（Gitの差分対象だけ）
          // ※ checkout scm 済み前提。Git for Windows がPATHにあること
          bat '''
            @echo off
            git diff --name-only HEAD~1 HEAD > diff_file_list.txt
            for %%A in (diff_file_list.txt) do if %%~zA==0 (
              echo [WARN] diff_file_list.txt is empty. Fallback to full analysis.
            )
          '''

          // 1) BuildSpec 生成（あなたが提示した buildCommand を “壊れていない形” に修正済み）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: '"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe" "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln" /t:Rebuild',
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat 'if not exist kwinject.out exit /b 1'
          bat 'for %%A in (kwinject.out) do if %%~zA==0 exit /b 1'

          // 2) 差分解析（Diff Analysis）
          //    - differentialAnalysisConfig が “差分解析の肝”
          //    - gitPreviousCommit は HEAD~1 を基本に
          klocworkIncremental([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList: 'diff_file_list.txt',
              diffType: 'git',
              gitPreviousCommit: 'HEAD~1'
            ],
            incrementalAnalysis: false,
            projectDir: '',
            reportFile: ''
          ])

          // Jenkinsのビルド説明欄に導線（任意）
          script {
            currentBuild.description = "Klocwork Diff: project=${env.KW_PROJECT} / build=${env.KW_BUILD_NAME}"
          }
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list.txt,kwtables/**', onlyIfSuccessful: false
    }
  }
}
