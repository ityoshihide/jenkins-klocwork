pipeline {
  agent any

  environment {
    // Klocworkツール（kwinject.exe / kwbuildproject.exe / kwadmin.exe）
    KW_TOOLS = 'C:\\Klocwork\\Validate_25.2\\kwbuildtools\\bin'
    // .sln があるフォルダ（最初の *.sln を自動選択）
    SLN_DIR  = 'C:\\Klocwork\\Command Line 25.2\\samples\\demosthenes\\vs2022'
    // PATH に Klocwork CLI を追加
    PATH     = "${env.PATH};${env.KW_TOOLS}"
  }

  stages {
    stage('Resolve MSBuild & .sln (PowerShell)') {
      steps {
        powershell '''
          $ErrorActionPreference = "Stop"

          Write-Host "[INFO] Resolving MSBuild with vswhere / PATH ..."
          $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\\Installer\\vswhere.exe"
          $msbuild = $null

          if (Test-Path $vswhere) {
            $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\\**\\Bin\\MSBuild.exe" | Select-Object -First 1
          }
          if (-not $msbuild) {
            $cmd = Get-Command msbuild.exe -ErrorAction SilentlyContinue
            if ($cmd) { $msbuild = $cmd.Path }
          }
          if (-not $msbuild) {
            throw "[ERROR] MSBuild.exe not found. Install VS Build Tools or add MSBuild to PATH."
          }
          Write-Host "[INFO] MSBuild: $msbuild"

          Write-Host "[INFO] Finding .sln under: ${env:SLN_DIR}"
          $sln = Get-ChildItem -LiteralPath ${env:SLN_DIR} -Filter *.sln -File -ErrorAction SilentlyContinue | Select-Object -First 1
          if (-not $sln) {
            throw "[ERROR] No .sln file found in '${env:SLN_DIR}'."
          }
          $solution = $sln.FullName
          Write-Host "[INFO] Solution: $solution"

          # export for later bat stages
          Set-Content -Path "build.env" -Value @(
            "MSBUILD=$msbuild"
            "SOLUTION=$solution"
          )
        '''
      }
    }

    stage('Build with kwinject') {
      steps {
        bat '''
        @echo off
        if not exist "%KW_TOOLS%\\kwinject.exe" (
          echo [ERROR] kwinject.exe not found: "%KW_TOOLS%"
          exit /b 1
        )
        if not exist build.env (
          echo [ERROR] build.env missing. Resolve stage didn't run?
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
      when { expression { return false } } // 使うとき true にしてURL/Project設定
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
