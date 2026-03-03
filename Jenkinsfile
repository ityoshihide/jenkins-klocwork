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

          // === ここを置き換え（// 1) kwinject 相当～// 4)Sync） ===
          p4StaticAnalysis([
            analysisType: 'Baseline',
            buildCaptureFile: '',
            buildCmd: '"C:\\\\Program Files\\\\Microsoft Visual Studio\\\\2022\\\\Professional\\\\MSBuild\\\\Current\\\\Bin\\\\MSBuild.exe" "C:\\\\Klocwork\\\\CommandLine25.4\\\\samples\\\\demosthenes\\\\vs2022\\\\4.sln" /t:Rebuild',
            enableQualityGate: false,
            engine: 'Klocwork',
            restrictionFileList: '',
            scanBuildName: '$BUILD_TAG',
            usingBuildCaptureFile: false,
            usingBuildCmd: true,
            validateProjectName: '',
            validateProjectURL: 'http://localhost:2540/'
          ])
          // === 置き換えここまで ===
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
