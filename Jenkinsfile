pipeline {
  agent any

  environment {
    // Klocwork tools (contains kwinject.exe, kwbuildproject.exe, kwadmin.exe)
    KW_TOOLS = 'C:\\Klocwork\\Validate_25.2\\kwbuildtools\\bin'
    // Folder that contains the solution (.sln). We will auto-pick the first *.sln in this folder.
    SLN_DIR  = 'C:\\Klocwork\\Command Line 25.2\\samples\\demosthenes\\vs2022'
    // Extend PATH so Jenkins can find Klocwork CLI
    PATH     = "${env.PATH};${env.KW_TOOLS}"
  }

  stages {
    stage('Checkout (optional)') {
      when { expression { return false } } // Disable if you do not need to fetch any repo.
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

    stage('Resolve MSBuild & Solution') {
      steps {
        bat '''
        echo [INFO] Resolving MSBuild path (VS2022)...
        set "VSWHERE=%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
        if exist "%VSWHERE%" (
          for /f "usebackq tokens=*" %%A in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe"`) do (
            set "MSBUILD=%%A"
          )
        )

        if not defined MSBUILD (
          echo [WARN] vswhere not found or MSBuild not located. Falling back to PATH lookup...
          for /f "delims=" %%A in ('where MSBuild.exe 2^>nul') do (
            set "MSBUILD=%%A"
            goto :FOUND_MSBUILD
          )
          echo [ERROR] MSBuild.exe not found. Please install VS Build Tools or add MSBuild to PATH.
          exit /b 1
        )
        :FOUND_MSBUILD
        echo [INFO] MSBuild resolved to: %MSBUILD%

        echo [INFO] Detecting .sln under: "%SLN_DIR%"
        set "SOLUTION="
        for %%F in ("%SLN_DIR%\\*.sln") do (
          set "SOLUTION=%%~fF"
          goto :FOUND_SOLUTION
        )
        echo [ERROR] No .sln file found in "%SLN_DIR%".
        exit /b 1
        :FOUND_SOLUTION
        echo [INFO] Using solution: %SOLUTION%

        echo MSBUILD=%MSBUILD%> build.env
        echo SOLUTION=%SOLUTION%>> build.env
        '''
      }
    }

    stage('Run kwinject') {
      steps {
        bat '''
        if not exist "%KW_TOOLS%\\kwinject.exe" (
          echo [ERROR] kwinject.exe not found at "%KW_TOOLS%".
          exit /b 1
        )

        call build.env
        echo [INFO] Running kwinject...
        del /q kwinject.out 2>nul

        "%KW_TOOLS%\\kwinject.exe" --output kwinject.out -- ^
          "%MSBUILD%" "%SOLUTION%" /t:Rebuild /p:Configuration=Release

        set "RC=%ERRORLEVEL%"
        if %RC% NEQ 0 (
          echo [ERROR] Build (wrapped by kwinject) failed with code %RC%.
          exit /b %RC%
        )
        '''
      }
    }

    stage('Verify kwinject.out') {
      steps {
        bat '''
        if not exist kwinject.out (
          echo [ERROR] kwinject.out not found.
          exit /b 1
        )
        for %%A in (kwinject.out) do (
          if %%~zA==0 (
            echo [ERROR] kwinject.out is empty.
            exit /b 1
          )
        )
        echo [INFO] kwinject.out looks good.
        '''
      }
    }

    stage('Run kwbuildproject') {
      steps {
        bat '''
        if not exist "%KW_TOOLS%\\kwbuildproject.exe" (
          echo [ERROR] kwbuildproject.exe not found at "%KW_TOOLS%".
          exit /b 1
        )
        rmdir /s /q kwtables 2>nul
        "%KW_TOOLS%\\kwbuildproject.exe" --tables-directory kwtables kwinject.out
        set "RC=%ERRORLEVEL%"
        if %RC% NEQ 0 (
          echo [ERROR] kwbuildproject failed with code %RC%.
          exit /b %RC%
        )
        echo [INFO] kwtables generated.
        '''
      }
    }

    stage('Run kwadmin (optional)') {
      when { expression { return false } } // Enable and fill in server details if needed
      steps {
        bat '''
        if not exist "%KW_TOOLS%\\kwadmin.exe" (
          echo [ERROR] kwadmin.exe not found at "%KW_TOOLS%".
          exit /b 1
        )
        set "KW_URL=http://<kwserver>:<port>"
        set "KW_PROJECT=<project_name>"
        "%KW_TOOLS%\\kwadmin.exe" --url %KW_URL% load %KW_PROJECT% kwtables
        '''
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**,build.env', onlyIfSuccessful: false
    }
  }
}
