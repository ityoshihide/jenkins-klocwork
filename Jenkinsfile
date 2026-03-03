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
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**', onlyIfSuccessful: false
    }
  }
}
