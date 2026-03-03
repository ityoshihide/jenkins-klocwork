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
        // 毎回クリーン（必要なら残す）
        bat 'if exist kwinject.out del /f /q kwinject.out'
        bat 'if exist kwtables rmdir /s /q kwtables'

        // p4StaticAnalysis に統一
        p4StaticAnalysis([
          analysisType: 'Baseline',
          buildCaptureFile: '',
          buildCmd: '"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe" "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln" /t:Rebuild',
          enableQualityGate: false,
          engine: 'Klocwork',
          restrictionFileList: '',
          scanBuildName: "${env.BUILD_TAG}",
          usingBuildCaptureFile: false,
          usingBuildCmd: true,
          validateProjectName: 'jenkins_demo',
          // あなたの前提：--url はプロジェクトURLまで含める
          validateProjectURL: 'http://localhost:2540/jenkins_demo'
        ])
      }
    }
  }

  post {
    always {
      // p4StaticAnalysis が生成する成果物の実体に合わせて必要なら調整
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**', onlyIfSuccessful: false, allowEmptyArchive: true
    }
  }
}
