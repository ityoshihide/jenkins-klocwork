pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  stages {
    stage('Klocwork (Plugin)') {
      steps {
        // Klocworkサーバ設定（Manage Jenkins -> System の "Validateサーバー" を参照）
        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: '',
          serverConfig: 'Validateサーバー',
          serverProject: 'jenkins_demo'
        ) {

          // （任意だけど安定）前回の解析テーブルを削除
          bat 'if exist kwtables rmdir /s /q kwtables'

          // 1) kwinject 相当：ビルド情報キャプチャ
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: '"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe" "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln" /t:Rebuild',
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // 2) kwbuildproject 相当：解析実行（tables生成）
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

          // 3) kwadmin load 相当：結果をValidateへロード
          klocworkIntegrationStep2(
            reportConfig: [displayChart: false],
            serverConfig: [additionalOpts: '', buildName: '', tablesDir: 'kwtables']
          )
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
