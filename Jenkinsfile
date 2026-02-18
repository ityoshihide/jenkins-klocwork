pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    KW_BASE_URL = 'http://192.168.137.1:2540'
    KW_PROJECT  = 'jenkins_demo'
  }

  stages {

    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Klocwork (Plugin)') {
      steps {
        script {
          // Klocwork側の build_name を毎回変える（例: jenkins-klocwork-pipeline-20）
          env.KW_BUILD_NAME = "jenkins-${env.JOB_NAME}-${env.BUILD_NUMBER}"
        }

        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: 'C:\\Users\\MSY11199\\.klocwork\\ltoken',
          serverConfig: 'Validateサーバー',
          serverProject: "${env.KW_PROJECT}"
        ) {

          // 毎回クリーン
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'

          // 1) kwinject 相当
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

          // 2) 解析（tables生成）
          klocworkIntegrationStep1([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            disableKwdeploy: false,
            duplicateFrom: '',
            ignoreCompileErrors: true,
            importConfig: '',
            incrementalAnalysis: false,
            tablesDir: 'kwtables'
          ])

          // 3) Load（buildName を明示）
          klocworkIntegrationStep2(
            reportConfig: [displayChart: true],
            serverConfig: [additionalOpts: '', buildName: "${env.KW_BUILD_NAME}", tablesDir: 'kwtables']
          )

          // Jenkinsのビルド画面（説明欄）にKlocwork結果への導線を表示（Syncより前に実行）
          script {
            def projectUrl = "${env.KW_BASE_URL}/${env.KW_PROJECT}"
            currentBuild.description =
              "Klocwork: <a href='${projectUrl}'>${env.KW_PROJECT}</a> / build=${env.KW_BUILD_NAME}"
          }

          // 4) Sync（最後）
          klocworkIssueSync([
            additionalOpts: '',
            dryRun: false,
            lastSync: '03-00-0000 00:00:00',
            projectRegexp: '',
            statusAnalyze: true,
            statusDefer: true,
            statusFilter: true,
            statusFix: true,
            statusFixInLaterRelease: false,
            statusFixInNextRelease: true,
            statusIgnore: true,
            statusNotAProblem: true
          ])
        }
      }
    }
  }

  post {
    always {
      // 成果物保存
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**', onlyIfSuccessful: false
    }
  }
}
