pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT   = 'C:\\msys64'
    MAKE_WORKDIR = '.'
    MAKE_ARGS    = 'all'

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    KW_BUILD_SPEC = 'kwinject.out'

    // Warnings NG に渡す
    KW_ISSUES_JSON  = 'kw_issues.json'
    KW_ISSUES_SARIF = 'kw_issues.sarif'

    // HTMLレポート
    KW_REPORT_DIR   = 'kw_report'
    KW_SEV_PIE_HTML = 'severity_pie.html'
    KW_HISTORY_HTML = 'history_bar.html'
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

    // ★追加：.kwlp/.kwps が無い場合のみ kwcheck create（オプション無し）
    stage('Ensure Klocwork Local Project') {
      steps {
        script {
          def kwlpDir = "${env.WORKSPACE}\\.kwlp"
          def kwpsDir = "${env.WORKSPACE}\\.kwps"

          def kwlpOk = fileExists(kwlpDir)
          def kwpsOk = fileExists(kwpsDir)

          if (!kwlpOk || !kwpsOk) {
            echo "[INFO] .kwlp/.kwps not found. Running: kwcheck create"

            bat """
              @echo off
              cd /d "%WORKSPACE%"

              kwcheck --version
              kwcheck create
            """
          } else {
            echo "[INFO] .kwlp/.kwps already exist. Skipping kwcheck create."
          }
        }
      }
    }

    // ★クリーンビルド（.kwlp/.kwpsは触らない）
    stage('Clean build outputs (force rebuild)') {
      steps {
        bat """
          @echo off
          cd /d "%WORKSPACE%\\%MAKE_WORKDIR%"

          echo [INFO] Clean build outputs to avoid 'Nothing to be done'
          if exist out (
            del /q out\\*.o 2>nul
            del /q out\\*.obj 2>nul
            del /q out\\demosthenes 2>nul
            del /q out\\demosthenes.exe 2>nul
          )
          del /q core* 2>nul
          echo [INFO] Done.
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
              // ★差分ではなく、全体解析に固定
              klocworkIncremental([
                additionalOpts     : '',
                buildSpec          : "${env.KW_BUILD_SPEC}",
                cleanupProject     : false,
                incrementalAnalysis: false,
                projectDir         : '',
                reportFile         : ''
              ])
            }
          }
        }
      }
    }

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

    // ★追加：severityCode 円グラフ + History 棒グラフ(自作) を生成
    stage('Generate Charts (severity pie + history bar)') {
      steps {
        powershell '''
          $ws = $env:WORKSPACE
          $jsonPath = Join-Path $ws $env:KW_ISSUES_JSON

          $reportDir = Join-Path $ws $env:KW_REPORT_DIR
          New-Item -ItemType Directory -Force -Path $reportDir | Out-Null

          if (!(Test-Path $jsonPath)) {
            Write-Host "[WARN] kw_issues.json not found. Skip chart generation."
            exit 0
          }

          $raw = Get-Content -Path $jsonPath -Raw
          if ([string]::IsNullOrWhiteSpace($raw)) {
            Write-Host "[WARN] kw_issues.json is empty. Skip chart generation."
            exit 0
          }

          try { $obj = $raw | ConvertFrom-Json } catch {
            Write-Host "[WARN] JSON parse failed. Skip chart generation."
            exit 0
          }

          $issues = $null
          if ($obj -is [System.Array]) { $issues = $obj }
          elseif ($obj.issues) { $issues = $obj.issues }
          elseif ($obj.results) { $issues = $obj.results }
          elseif ($obj.items) { $issues = $obj.items }

          if ($null -eq $issues) {
            Write-Host "[WARN] Could not locate issues array. Skip chart generation."
            exit 0
          }

          # ---------- severityCode 集計 ----------
          $sevCounts = @{}
          foreach ($it in $issues) {
            $sev = $null
            foreach ($k in @('severityCode','severity_code','severity')) { if ($it.$k) { $sev = [string]$it.$k; break } }
            if ([string]::IsNullOrWhiteSpace($sev)) { $sev = 'UNKNOWN' }

            if (-not $sevCounts.ContainsKey($sev)) { $sevCounts[$sev] = 0 }
            $sevCounts[$sev]++
          }

          $labels = @($sevCounts.Keys | Sort-Object)
          $values = @($labels | ForEach-Object { [int]$sevCounts[$_] })

          # ---------- History(棒) 用：ビルド番号と total を蓄積 ----------
          $total = ($values | Measure-Object -Sum).Sum
          $buildNo = $env:BUILD_NUMBER

          $historyJsonPath = Join-Path $reportDir "history.json"
          $history = @()

          if (Test-Path $historyJsonPath) {
            try {
              $history = (Get-Content $historyJsonPath -Raw | ConvertFrom-Json)
              if ($null -eq $history) { $history = @() }
              if ($history -isnot [System.Array]) { $history = @($history) }
            } catch { $history = @() }
          }

          # 同一ビルド番号があれば置換
          $history = @($history | Where-Object { $_.build -ne [int]$buildNo })
          $history += [pscustomobject]@{ build = [int]$buildNo; total = [int]$total }
          $history = @($history | Sort-Object build)

          $history | ConvertTo-Json -Depth 5 | Set-Content -Path $historyJsonPath -Encoding UTF8

          # ---------- HTML生成（Chart.js CDN）----------
          $chartJs = "https://cdn.jsdelivr.net/npm/chart.js"

          # severity pie
          $pieHtmlPath = Join-Path $reportDir $env:KW_SEV_PIE_HTML
          $pieLabelsJson = ($labels | ConvertTo-Json -Compress)
          $pieValuesJson = ($values | ConvertTo-Json -Compress)

          @"
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <title>Klocwork severityCode Overview</title>
  <script src="$chartJs"></script>
  <style>
    body { font-family: Arial, sans-serif; margin: 16px; }
    .wrap { max-width: 900px; }
    h2 { margin: 0 0 12px 0; }
  </style>
</head>
<body>
  <div class="wrap">
    <h2>Overview (severityCode)</h2>
    <p>Total issues: $total</p>
    <canvas id="pie" height="140"></canvas>
  </div>

  <script>
    const labels = $pieLabelsJson;
    const values = $pieValuesJson;

    new Chart(document.getElementById('pie'), {
      type: 'pie',
      data: {
        labels,
        datasets: [{ data: values }]
      },
      options: {
        responsive: true,
        plugins: {
          legend: { position: 'right' }
        }
      }
    });
  </script>
</body>
</html>
"@ | Set-Content -Path $pieHtmlPath -Encoding UTF8

          # history bar
          $histHtmlPath = Join-Path $reportDir $env:KW_HISTORY_HTML
          $histLabels = @($history | ForEach-Object { "Build " + $_.build })
          $histValues = @($history | ForEach-Object { [int]$_.total })

          $histLabelsJson = ($histLabels | ConvertTo-Json -Compress)
          $histValuesJson = ($histValues | ConvertTo-Json -Compress)

          @"
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <title>Klocwork History</title>
  <script src="$chartJs"></script>
  <style>
    body { font-family: Arial, sans-serif; margin: 16px; }
    .wrap { max-width: 1000px; }
    h2 { margin: 0 0 12px 0; }
  </style>
</head>
<body>
  <div class="wrap">
    <h2>History (Total issues per build) - Bar</h2>
    <canvas id="bar" height="120"></canvas>
  </div>

  <script>
    const labels = $histLabelsJson;
    const values = $histValuesJson;

    new Chart(document.getElementById('bar'), {
      type: 'bar',
      data: {
        labels,
        datasets: [{ label: 'Total', data: values }]
      },
      options: {
        responsive: true,
        scales: {
          y: { beginAtZero: true }
        }
      }
    });
  </script>
</body>
</html>
"@ | Set-Content -Path $histHtmlPath -Encoding UTF8

          Write-Host "[INFO] Wrote charts to $reportDir"
          Write-Host ("[INFO] severityCode breakdown: " + ($sevCounts.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value)" } | Out-String))
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

    // ★追加：HTMLとして表示（HTML Publisher plugin が入っている場合）
    stage('Publish Charts (HTML)') {
      steps {
        script {
          // HTML Publisher plugin が無い環境でもビルドを落とさない
          try {
            publishHTML(target: [
              allowMissing: true,
              alwaysLinkToLastBuild: true,
              keepAll: true,
              reportDir: "${env.KW_REPORT_DIR}",
              reportFiles: "${env.KW_SEV_PIE_HTML},${env.KW_HISTORY_HTML}",
              reportName: "Klocwork Charts"
            ])
          } catch (err) {
            echo "[WARN] publishHTML failed (maybe HTML Publisher plugin not installed). Charts are still archived as artifacts."
          }
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_REPORT_DIR}/**", allowEmptyArchive: true
    }
  }
}
