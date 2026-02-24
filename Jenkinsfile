pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  stages {

    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Klocwork (Plugin)') {
      steps {
        klocworkWrapper(
          installConfig: '-- なし --',
          // ★ここを明示：本来の場所（あなたが提示したパス）
          ltoken: 'C:\\Users\\MSY11199\\.klocwork\\ltoken',
          serverConfig: 'Validateサーバー',
          serverProject: 'jenkins_demo'
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

          // 3) Load
          klocworkIntegrationStep2(
            reportConfig: [displayChart: false],
            serverConfig: [additionalOpts: '', buildName: '', tablesDir: 'kwtables']
          )

          // 4)Sync
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
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**', onlyIfSuccessful: false
    }
  }
}
