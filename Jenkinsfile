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
    MAKE_ARGS     = ''

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    DIFF_FILE_LIST = 'diff_file_list.txt'
    KW_BUILD_SPEC  = 'kwinject.out'

    // ★Warnings NG 用
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

    stage('Decide previous commit') {
      steps {
        script {
          // 優先順：前回成功 → 前回 → HEAD~1 → (取れなければ空)
          def prev = env.GIT_PREVIOUS_SUCCESSFUL_COMMIT
          if (!prev?.trim()) prev = env.GIT_PREVIOUS_COMMIT

          if (!prev?.trim()) {
            // HEAD~1 を試す（初回ビルドだと失敗することあり）
            def rc = bat(returnStatus: true, script: 'git rev-parse HEAD~1 > .prev_commit 2>nul')
            if (rc == 0) {
              prev = readFile('.prev_commit').trim()
            }
          }

          if (prev?.trim()) {
            env.KW_PREV_COMMIT = prev.trim()
            echo "Using KW_PREV_COMMIT=${env.KW_PREV_COMMIT}"
          } else {
            env.KW_PREV_COMMIT = ''
            echo "KW_PREV_COMMIT not available -> fallback to full analysis"
          }
        }
      }
    }

    stage('Create diff file list') {
      steps {
        script {
          if (!env.KW_PREV_COMMIT?.trim()) {
            // 差分基準が無いなら全ファイルにしておく（=差分解析は使わない想定）
            bat """
              @echo off
              git ls-files > "%DIFF_FILE_LIST%"
              echo [INFO] diff file list (fallback: all files):
              type "%DIFF_FILE_LIST%"
            """
          } else {
            bat """
              @echo off
              git diff --name-only %KW_PREV_COMMIT% HEAD > "%DIFF_FILE_LIST%"
              for %%A in ("%DIFF_FILE_LIST%") do if %%~zA==0 (
                git ls-files > "%DIFF_FILE_LIST%"
              )
              echo [INFO] diff file list (%KW_PREV_COMMIT%..HEAD):
              type "%DIFF_FILE_LIST%"
            """
          }
        }
      }
    }

    stage('Klocwork Analysis') {
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
            // BuildSpec生成（kwinjectを手動実行）
            bat """
              @echo off
              cd /d "%WORKSPACE%\\%MAKE_WORKDIR%"

              echo [INFO] kwinject version
              kwinject --version

              if exist "%WORKSPACE%\\%KW_BUILD_SPEC%" del /f /q "%WORKSPACE%\\%KW_BUILD_SPEC%"

              echo [INFO] generating build spec: %KW_BUILD_SPEC%
              echo [INFO] run: kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
              kwinject --output "%WORKSPACE%\\%KW_BUILD_SPEC%" "%MAKE_EXE%" %MAKE_ARGS%
            """

            script {
              if (env.KW_PREV_COMMIT?.trim()) {
                // ★差分解析（previous commit を必ずハッシュで渡す）
                klocworkIncremental([
                  additionalOpts: '',
                  buildSpec     : "${env.KW_BUILD_SPEC}",
                  cleanupProject: false,
                  differentialAnalysisConfig: [
                    diffFileList     : "${env.DIFF_FILE_LIST}",
                    diffType         : 'git',
                    gitPreviousCommit: "${env.KW_PREV_COMMIT}"
                  ],
                  incrementalAnalysis: true,
                  projectDir        : '',
                  reportFile        : ''
                ])
              } else {
                // previous commit が取れない時は、無理に差分解析せずフル解析
                klocworkIncremental([
                  additionalOpts: '',
                  buildSpec     : "${env.KW_BUILD_SPEC}",
                  cleanupProject: false,
                  incrementalAnalysis: false,
                  projectDir        : '',
                  reportFile        : ''
                ])
              }
            }
          }
        }
      }
    }

    // ★追加：issues JSON -> SARIF
    stage('Export issues JSON -> SARIF (for Warnings NG)') {
      steps {
        bat """
          @echo off
          echo [INFO] Export Klocwork issues to %KW_ISSUES_JSON%
          kwciagent list --system --project-dir "%WORKSPACE%\\.kwlp" --license-host 192.168.137.1 --license-port 27000 -F json > "%KW_ISSUES_JSON%"
          echo [INFO] JSON size:
          for %%A in ("%KW_ISSUES_JSON%") do echo %%~zA bytes
        """

        powershell '''
          $jsonPath  = Join-Path $env:WORKSPACE $env:KW_ISSUES_JSON
          $sarifPath = Join-Path $env:WORKSPACE $env:KW_ISSUES_SARIF

          function Write-EmptySarif($path) {
            $emptyObj = @{
              '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
              version='2.1.0'
              runs=@(@{ tool=@{ driver=@{ name='Klocwork'; rules=@() } }; results=@() })
            }
            $emptyObj | ConvertTo-Json -Depth 20 | Set-Content -Path $path -Encoding UTF8
          }

          if (!(Test-Path $jsonPath)) { Write-Host "[WARN] JSON not found: $jsonPath"; Write-EmptySarif $sarifPath; exit 0 }

          $raw = Get-Content -Path $jsonPath -Raw
          if ([string]::IsNullOrWhiteSpace($raw)) { Write-Host "[WARN] JSON empty"; Write-EmptySarif $sarifPath; exit 0 }

          try { $obj = $raw | ConvertFrom-Json } catch {
            Write-Host "[ERROR] ConvertFrom-Json failed. Dump first 500 chars:"
            Write-Host ($raw.Substring(0, [Math]::Min(500, $raw.Length)))
            Write-EmptySarif $sarifPath
            exit 0
          }

          $issues = $null
          if ($obj -is [System.Array]) { $issues = $obj }
          elseif ($obj.issues) { $issues = $obj.issues }
          elseif ($obj.results) { $issues = $obj.results }
          elseif ($obj.items) { $issues = $obj.items }

          if ($null -eq $issues) {
            Write-Host "[WARN] Could not find issues array in JSON. Keys:"
            $obj.PSObject.Properties.Name | ForEach-Object { Write-Host " - $_" }
            Write-EmptySarif $sarifPath
            exit 0
          }

          $results = New-Object System.Collections.Generic.List[Object]

          foreach ($it in $issues) {
            $file = $null
            foreach ($k in @('file','path','filename','sourceFile','source_file')) { if ($it.$k) { $file = [string]$it.$k; break } }
            if ([string]::IsNullOrWhiteSpace($file)) { $file = 'unknown' }

            $rule = $null
            foreach ($k in @('code','checker','rule','ruleId','id')) { if ($it.$k) { $rule = [string]$it.$k; break } }
            if ([string]::IsNullOrWhiteSpace($rule)) { $rule = 'KLOCWORK' }

            $msg = $null
            foreach ($k in @('message','msg','description','text')) { if ($it.$k) { $msg = [string]$it.$k; break } }
            if ([string]::IsNullOrWhiteSpace($msg)) { $msg = '' }

            $line = 1
            foreach ($k in @('line','lineNumber','line_number','startLine')) {
              if ($it.$k) { $tmp = 0; if ([int]::TryParse([string]$it.$k, [ref]$tmp) -and $tmp -gt 0) { $line = $tmp }; break }
            }

            $results.Add(@{
              ruleId  = $rule
              message = @{ text = $msg }
              locations = @(@{
                physicalLocation = @{
                  artifactLocation = @{ uri = $file }
                  region = @{ startLine = $line }
                }
              })
            })
          }

          $sarifObj = @{
            '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version='2.1.0'
            runs=@(@{
              tool=@{ driver=@{ name='Klocwork'; rules=@() } }
              results=$results
            })
          }

          $sarifObj | ConvertTo-Json -Depth 20 | Set-Content -Path $sarifPath -Encoding UTF8
          Write-Host "[INFO] Wrote SARIF: $sarifPath"
          Write-Host ("[INFO] SARIF results: {0}" -f $results.Count)
        '''
      }
    }

    // ★追加：JenkinsのWarningsに表示
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
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
