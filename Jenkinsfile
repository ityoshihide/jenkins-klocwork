pipeline {
  agent any

  environment {
    // Klocworkツール一式（kwinject.exe / kwbuildproject.exe / kwadmin.exe）
    KW_TOOLS = 'C:\\Klocwork\\Validate_25.2\\kwbuildtools\\bin'
    // .slnが入っているフォルダ（最初の *.sln を自動選択）
    SLN_DIR  = 'C:\\Klocwork\\Command Line 25.2\\samples\\demosthenes\\vs2022'
    // PATHにKlocwork CLIを追加
    PATH     = "${env.PATH};${env.KW_TOOLS}"
  }

  stages {
    stage('Resolve MSBuild & .sln') {
      steps {
        bat '''
        @echo off
        setlocal EnableExtensions EnableDelayedExpansion

        rem --- Resolve MSBuild ---
        set "MSBUILD="
        set "VSWHERE=%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
        if exist "!VSWHERE!" (
          for /f "usebackq delims=" %%A in (`"!VSWHERE!" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe"`) do (
            set "MSBUILD=%%~fA"
          )
        )
        if not defined MSBUILD (
          for /f "delims=" %%A in ('where MSBuild.exe 2^>nul') do (
            set "MSBUILD=%%~fA"
            goto FOUND_MSBUILD
          )
          echo [ERROR] MSBuild.exe not found. Install VS Build Tools or add to PATH.
          exit /b 1
        )
        :FOUND_MSBUILD
        echo [INFO] MSBuild: !MSBUILD!

        rem --- Pick first .sln under SLN_DIR ---
        set "SOLUTION="
        for /f "delims=" %%F in ('dir /b /a:-d "%SLN_DIR%\\*.sln" 2^>nul') do (
          set "SOLUTION=%SLN_DIR%\\%%~F"
          goto FOUND_SOLUTION
        )
        echo [ERROR] No .sln in "%SLN_DIR%".
        exit /b 1
        :FOUND_SOLUTION
        echo [INFO] Solution: !SOLUTION!

        rem --- Export for next stages ---
        >  build.env echo MSBUILD=!MSBUILD!
        >> build.env echo SOLUTION=!SOLUTION!

        endlocal
        '''
      }
    }

    stage('Build with kwinject') {
      steps {
        bat '''
        @echo off
        setlocal
        if not exist "%KW_TOOLS%\\kwinject.exe" (
          echo [ERROR] kwinject.exe not found: "%KW_TOOLS%"
          exit /b 1
        )
        call build.env

        echo [INFO] Running kwinject...
        del /q kwinject.out 2>nul

        "%KW_TOOLS%\\kwinject.exe" --output kwinject.out -- ^
          "%MSBUILD%" "%SOLUTION%" /t:Rebuild /p:Configuration=Release
        if errorlevel 1 (
          echo [ERROR] Build failed (kwinject wrapper).
          exit /b 1
        )

        if not exist kwinject.out (
          echo [ERROR] kwinject.out missing.
          exit /b 1
        )
        for %%A in (kwinject.out) do if %%~zA==0 (
          echo [ERROR] kwinject.out is empty.
          exit /b 1
        )
        echo [INFO] kwinject.out OK.
        endlocal
        '''
      }
    }

    stage('kwbuildproject') {
      steps {
        bat '''
        @echo off
        if not exist "%KW_TOOLS%\\kwbuildproject.exe" (
          echo [ERROR] kwbuildproject.exe not found: "%KW_TOOLS%"
          exit /b 1
        )
        rmdir /s /q kwtables 2>nul
        "%KW_TOOLS%\\kwbuildproject.exe" --tables-directory kwtables kwinject.out
        if errorlevel 1 (
          echo [ERROR] kwbuildproject failed.
          exit /b 1
        )
        echo [INFO] kwtables generated.
        '''
      }
    }

    stage('kwadmin upload (optional)') {
      when { expression { return false } } // 使う場合は true にしてURL/Project設定
      steps {
        bat '''
        @echo off
        if not exist "%KW_TOOLS%\\kwadmin.exe" (
          echo [ERROR] kwadmin.exe not found: "%KW_TOOLS%"
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
      archiveArtifacts artifacts: 'build.env,kwinject.out,kwtables/**', onlyIfSuccessful: false
    }
  }
}
