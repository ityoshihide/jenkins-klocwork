pipeline {
  agent any
  options { skipDefaultCheckout(true) } // 余計なSCMチェックアウトを無効化

  environment {
    KW_BIN   = 'C:\\Klocwork\\Validate_25.2\\kwbuildtools\\bin'
    SLN      = 'C:\\Klocwork\\Command Line 25.2\\samples\\demosthenes\\vs2022\\4.sln'
    MSBUILD  = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    KW_URL   = 'http://localhost:2520'
    PROJECT  = 'sample'
    CONFIG   = 'Debug'
    PLATFORM = 'x64'
  }

  stages {
    stage('1) kwinject') {
      steps {
        bat '''
        @echo off
        echo [kwinject] %SLN% (%CONFIG%|%PLATFORM%)
        "%KW_BIN%\\kwinject.exe" -- ^
          "%MSBUILD%" "%SLN%" /t:Rebuild /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%
        if errorlevel 1 exit /b 1
        if not exist kwinject.out (echo [ERROR] kwinject.out missing & exit /b 1)
        for %%A in (kwinject.out) do if %%~zA==0 (echo [ERROR] kwinject.out is empty & exit /b 1)
        '''
      }
    }

    stage('2) kwbuildproject') {
      steps {
        bat '''
        @echo off
        echo [kwbuildproject] -> tables
        rmdir /s /q tables 2>nul
        "%KW_BIN%\\kwbuildproject.exe" --url %KW_URL%/%PROJECT% -o tables kwinject.out -f
        if errorlevel 1 exit /b 1
        '''
      }
    }

    stage('3) kwadmin load') {
      steps {
        bat '''
        @echo off
        echo [kwadmin] load %PROJECT%
        "%KW_BIN%\\kwadmin.exe" --url %KW_URL% load %PROJECT% tables
        if errorlevel 1 exit /b 1
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

