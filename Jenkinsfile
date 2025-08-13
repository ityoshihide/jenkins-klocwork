pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  stages {
    stage('Klocwork run') {
      steps {
        bat '''
        @echo off
        set "KW_BIN=C:\\Klocwork\\Validate_25.2\\kwbuildtools\\bin"
        set "SLN=C:\\Klocwork\\Command Line 25.2\\samples\\demosthenes\\vs2022\\4.sln"
        set "KW_URL=http://localhost:2520"
        set "PROJECT=sample"

        echo [1/3] kwinject
        "%KW_BIN%\\kwinject.exe" msbuild "%SLN%" /t:rebuild /p:configuration=debug || exit /b 1

        echo [2/3] kwbuildproject
        "%KW_BIN%\\kwbuildproject.exe" --url %KW_URL%/%PROJECT% -o tables kwinject.out -f || exit /b 1

        echo [3/3] kwadmin load
        "%KW_BIN%\\kwadmin.exe" --url %KW_URL% load %PROJECT% tables || exit /b 1
        '''
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,tables/**', onlyIfSuccessful: false
    }
  }
}

