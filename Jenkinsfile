pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT    = 'C:\\msys64'
    MAKE_WORKDIR  = '.'
    MAKE_ARGS     = 'all'

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    KW_PROJECT_DIR = '.kwlp'
    KW_BUILD_SPEC  = 'kwinject.out'

    // Warnings NG に渡す
    KW_ISSUES_JSON  = 'kw_issues.json'
    KW_ISSUES_SARIF = 'kw_issues.sarif'
  }

  stages {
    stage('Checkout') {
      steps { checkout scm }
    }

    stage('Resolve make path') {
      steps {
        script {
          def c1 = "${env.MSYS2_ROOT}\\mingw64\\bin\\mingw32-make.exe"
          def c2 = "${env.MSYS2_ROOT}\\usr\\bin\\make.exe"
          if (fileExists(c1)) env.MAKE_EXE = c1
          else if (fileExists(c2)) env.MAKE_EXE = c2
          else error("make が見つかりません: ${c1} / ${c2}")
        }

        bat """
          @echo off
          echo [INFO] Using MAKE_EXE=%MAKE_EXE%
          "%MAKE_EXE%" --version
        """
      }
    }

    stage('Reset workspace Klocwork artifacts (FORCE FULL)') {
      steps {
        bat """
          @echo off
          cd /d "%WORKSPACE%"

          echo [INFO] Remove previous Klocwork working dir: %KW_PROJECT_DIR%
          if exist "%WORKSPACE%\\%KW_PROJECT_DIR%" rmdir /s /q "%WORKSPACE%\\%KW_PROJECT_DIR%"

          echo [INFO] Remove previous build spec: %KW_BUILD_SPEC%
          if exist "%WORKSPACE%\\%KW_BUILD_SPEC%" del /f /q "%WORKSPACE%\\%KW_BUILD_SPEC%"

          echo [INFO] Remove previous SARIF/JSON
          if exist "%WORKSPACE%\\%KW_ISSUES_JSON%" del /f /q "%WORKSPACE%\\%KW_ISSUES_JSON%"
          if exist "%WORKSPACE%\\%KW_ISSUES_SARIF%" del /f /q "%WORKSPACE%\\%KW_ISSUES_SARIF%"
        """
      }
    }

    stage('Klocwork Analysis (FULL)') {
      steps {
        withEnv([
          "PATH+MSYS2_USR=${env.MSYS2_ROOT}\\usr\\bin",
          "PATH+MSYS2_MINGW64=${env.MSYS2_ROOT}\\mingw64\\bin"
        ]) {
          klocworkWrapper(
            installConfig: '-- なし --',
            ltoken: "${env.KW_LTOKEN}",
            serverConfig: "${env.KW_SERVER_CONFIG}",
            serverProject: "${env.KW_SERVER_PROJECT}"
          ) {
            // 1) 必ずクリーンしてコンパイルを発生させる（ここが最重要）
            bat """
              @echo off
              cd /d "%WORKSPACE%\\%MAKE_WORKDIR%"

              echo [INFO] Pre-clean to force rebuild
              "%MAKE_EXE%" clean

              echo [INFO] kwinject version
              kwinject --version

              echo [INFO] generating build spec: %KW_BUILD_SPEC%
              echo [INFO] run: kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
              kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
            """

            // 2) kwciagent run が 0 で終わることを強制（失敗してたらそこで落とす）
            script {
              def rc = bat(returnStatus: true, script: """
                @echo off
                kwciagent set --project-dir "%WORKSPACE%\\%KW_PROJECT_DIR%" klocwork.host=192.168.137.1 klocwork.port=2540 klocwork.project=%KW_SERVER_PROJECT%
                kwciagent run --project-dir "%WORKSPACE%\\%KW_PROJECT_DIR%" --license-host 192.168.137.1 --license-port 27000 -Y -L --build-spec "%WORKSPACE%\\%KW_BUILD_SPEC%"
              """)
              if (rc != 0) {
                error("kwciagent run failed (rc=${rc}). Build spec may be empty. Check that compilation actually ran under kwinject.")
              }
            }

            // 3) 全指摘を JSON で取得（ここが “全体” の出口）
            bat """
              @echo off
              echo [INFO] Export Klocwork issues to %KW_ISSUES_JSON%
              kwciagent list --project-dir "%WORKSPACE%\\%KW_PROJECT_DIR%" --license-host 192.168.137.1 --license-port 27000 -F json > "%WORKSPACE%\\%KW_ISSUES_JSON%"
              for %%A in ("%WORKSPACE%\\%KW_ISSUES_JSON%") do echo [INFO] JSON size: %%~zA bytes
            """
          }
        }
      }
    }

    stage('Convert JSON -> SARIF (for Warnings NG)') {
      steps {
        powershell '''
          $jsonPath  = Join-Path $env:WORKSPACE $env:KW_ISSUES_JSON
          $sarifPath = Join-Path $env:WORKSPACE $env:KW_ISSUES_SARIF

          if (!(Test-Path $jsonPath)) {
            throw "[ERROR] JSON not found: $jsonPath"
          }

          $data = Get-Content $jsonPath -Raw | ConvertFrom-Json

          # Klocwork の JSON 形式は環境/バージョンで揺れるので「配列っぽいところ」を探す
          $issues = @()
          if ($data -is [System.Array]) { $issues = $data }
          elseif ($data.issues) { $issues = $data.issues }
          elseif ($data.result) { $issues = $data.result }
          else { $issues = @() }

          $results = @()

          foreach ($i in $issues) {
            # フィールド名が揺れても拾えるように候補を複数見る
            $file  = $i.file
            if (-not $file) { $file = $i.path }
            if (-not $file) { $file = $i.location?.file }

            $rule  = $i.code
            if (-not $rule) { $rule = $i.checker }
            if (-not $rule) { $rule = $i.ruleId }

            $msg   = $i.message
            if (-not $msg) { $msg = $i.msg }
            if (-not $msg) { $msg = $i.description }

            $line  = $i.line
            if (-not $line) { $line = $i.location?.line }
            if (-not $line) { $line = 1 }

            if (-not $file -or -not $rule -or -not $msg) { continue }

            # Warnings NG は workspace 内の相対パスの方が安定することが多い
            # 絶対パスだったら workspace 部分を落として相対化を試みる
            $uri = $file
            $ws = $env:WORKSPACE.TrimEnd('\\')
            if ($uri.StartsWith($ws, [System.StringComparison]::OrdinalIgnoreCase)) {
              $uri = $uri.Substring($ws.Length).TrimStart('\\')
              $uri = $uri -replace '\\\\','/'
            }

            $results += @{
              ruleId  = "$rule"
              message = @{ text = "$msg" }
              locations = @(@{
                physicalLocation = @{
                  artifactLocation = @{ uri = "$uri" }
                  region = @{ startLine = [int]$line }
                }
              })
            }
          }

          $sarif = @{
            '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version='2.1.0'
            runs=@(@{
              tool=@{ driver=@{ name='Klocwork'; rules=@() } }
              results=$results
            })
          } | ConvertTo-Json -Depth 50

          Set-Content -Path $sarifPath -Value $sarif -Encoding UTF8
          Write-Host "[INFO] Wrote SARIF: $sarifPath"
          Write-Host "[INFO] SARIF results: $($results.Count)"
        '''
      }
    }

    stage('Show issues as table in Jenkins (Warnings NG)') {
      steps {
        recordIssues(
          enabledForFailure: true,
          tools: [sarif(pattern: "${env.KW_ISSUES_SARIF}")]
        )
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
