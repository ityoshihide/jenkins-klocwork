
pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // ここは環境に合わせて調整
    KW_SERVER_URL   = 'http://192.168.137.1:2540'
    KW_PROJECT_NAME = 'jenkins_demo'

    // Warnings NG 用
    KW_ISSUES_JSON  = 'kw_issues.json'
    KW_ISSUES_SARIF = 'kw_issues.sarif'
  }

  stages {

    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Klocwork (Plugin)') {
      steps {
        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: 'C:\\Users\\MSY11199\\.klocwork\\ltoken',
          serverConfig: 'Validateサーバー',
          serverProject: 'jenkins_demo'
        ) {

          // 毎回クリーン
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'

          // 1) kwinject 相当
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: '"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe" "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln" /t:Rebuild',
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat 'if not exist kwinject.out exit /b 1'
          bat 'for %%A in (kwinject.out) do if %%~zA==0 exit /b 1'

          // 2) 解析（tables生成）
          klocworkIntegrationStep1([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            disableKwdeploy: false,
            duplicateFrom: '',
            ignoreCompileErrors: true,
            importConfig: '',
            incrementalAnalysis: false,
            tablesDir: 'kwtables'
          ])

          // 3) Load（displayChartはfalseのまま）
          klocworkIntegrationStep2(
            reportConfig: [displayChart: false],
            serverConfig: [additionalOpts: '', buildName: '', tablesDir: 'kwtables']
          )

          // 4) Sync
          klocworkIssueSync([
            additionalOpts: '',
            dryRun: false,
            lastSync: '03-00-0000 00:00:00',
            projectRegexp: '',
            statusAnalyze: true,
            statusDefer: true,
            statusFilter: true,
            statusFix: true,
            statusFixInLaterRelease: false,
            statusFixInNextRelease: true,
            statusIgnore: true,
            statusNotAProblem: true
          ])
        }
      }
    }

    stage('Export issues JSON -> SARIF (for Warnings NG)') {
      steps {
        bat """
          @echo off
          echo [INFO] Export Klocwork issues to %KW_ISSUES_JSON%
          kwciagent list --url "%KW_SERVER_URL%/%KW_PROJECT_NAME%" --system -F json > "%KW_ISSUES_JSON%"
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

          # kwciagent list の出力形に吸収
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

            $sev = $null
            foreach ($k in @('severityCode','severity','sev')) { if ($it.$k -ne $null) { $sev = [string]$it.$k; break } }

            $msg = $null
            foreach ($k in @('message','msg','description','text')) { if ($it.$k) { $msg = [string]$it.$k; break } }
            if ([string]::IsNullOrWhiteSpace($msg)) { $msg = '' }
            if (![string]::IsNullOrWhiteSpace($sev)) { $msg = "[severityCode=$sev] $msg" }

            $line = 1
            foreach ($k in @('line','lineNumber','line_number','startLine')) {
              if ($it.$k) { $tmp = 0; if ([int]::TryParse([string]$it.$k, [ref]$tmp) -and $tmp -gt 0) { $line = $tmp }; break }
            }

            $results.Add(@{
              ruleId  = $rule
              level   = 'warning'
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
      archiveArtifacts artifacts: 'kwinject.out,kwtables/**', onlyIfSuccessful: false
      archiveArtifacts artifacts: "${env.KW_ISSUES_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
