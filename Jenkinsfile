pipeline {
  agent any

  environment {
    KW_HOME = 'C:\\Klocwork\\kw' // Adjust to your actual installation path
    PATH = "${env.PATH};${env.KW_HOME}\\bin"
  }

  stages {
    stage('Checkout') {
      steps {
        checkout([$class: 'GitSCM',
          branches: [[name: '*/main']],
          userRemoteConfigs: [[
            url: 'https://github.com/ityoshihide/jenkins-klocwork.git',
            credentialsId: 'MyGithubCredentials'
          ]]
        ])
      }
    }

    stage('Run kwinject') {
      steps {
        echo '[INFO] Running kwinject on Windows...'
        bat '''
        if not exist "%KW_HOME%\\bin\\kwinject.exe" (
          echo ERROR: kwinject.exe not found & exit /b 1
        )
        rem --- Wrap your actual build command here ---
        kwinject --output kwinject.out -- ^
          msbuild YourSolution.sln /t:Rebuild /p:Configuration=Release
        '''
      }
    }

    stage('Verify kwinject.out') {
      steps {
        bat '''
        if not exist kwinject.out (
          echo ERROR: kwinject.out not found & exit /b 1
        )
        for %%A in (kwinject.out) do if %%~zA==0 (
          echo ERROR: kwinject.out is empty & exit /b 1
        )
        '''
      }
    }

    stage('Run kwbuildproject') {
      steps {
        bat '''
        kwbuildproject --tables-directory kwtables kwinject.out
        '''
      }
    }

    stage('Run kwadmin') {
      when { expression { return false } } // Enable this stage if needed
      steps {
        bat '''
        kwadmin --url http://<kwserver>:<port> load <project_name> kwtables
        '''
      }
    }
  }
}

