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

    // 自前ダッシュボード
    KW_SEV_CURRENT_JSON = 'kw_severity_current.json'
    KW_DASHBOARD_HTML   = 'kw_dashboard.html'
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
          kwciagent list --project-dir "%WORKSPACE%\\.kwlp" --license-host 192.168.137.1 --license-port 27000 -F json > "%KW_ISSUES_JSON%"
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

            # SARIFのseverityはここでは触らず、Warnings NGは「全部warning扱い」でも動く
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

    // ★追加：kw_issues.json の severityCode で「円 + 棒(推移)」ダッシュボード生成
    stage('Build Klocwork Dashboard (severityCode Pie + History Bar)') {
      steps {
        powershell '''
          $issuesPath   = Join-Path $env:WORKSPACE $env:KW_ISSUES_JSON
          $currentPath  = Join-Path $env:WORKSPACE $env:KW_SEV_CURRENT_JSON
          $htmlPath     = Join-Path $env:WORKSPACE $env:KW_DASHBOARD_HTML

          if (!(Test-Path $issuesPath)) {
            Write-Host "[WARN] kw_issues.json not found: $issuesPath"
            '{}' | Set-Content -Path $currentPath -Encoding UTF8
            "<html><body><h2>Klocwork Dashboard</h2><p>kw_issues.json not found.</p></body></html>" | Set-Content -Path $htmlPath -Encoding UTF8
            exit 0
          }

          $raw = Get-Content -Path $issuesPath -Raw
          if ([string]::IsNullOrWhiteSpace($raw)) {
            Write-Host "[WARN] kw_issues.json empty"
            '{}' | Set-Content -Path $currentPath -Encoding UTF8
            "<html><body><h2>Klocwork Dashboard</h2><p>kw_issues.json empty.</p></body></html>" | Set-Content -Path $htmlPath -Encoding UTF8
            exit 0
          }

          try { $obj = $raw | ConvertFrom-Json } catch {
            Write-Host "[WARN] ConvertFrom-Json failed"
            '{}' | Set-Content -Path $currentPath -Encoding UTF8
            "<html><body><h2>Klocwork Dashboard</h2><p>kw_issues.json parse failed.</p></body></html>" | Set-Content -Path $htmlPath -Encoding UTF8
            exit 0
          }

          $issues = $null
          if ($obj -is [System.Array]) { $issues = $obj }
          elseif ($obj.issues) { $issues = $obj.issues }
          elseif ($obj.results) { $issues = $obj.results }
          elseif ($obj.items) { $issues = $obj.items }
          if ($null -eq $issues) { $issues = @() }

          # severityCode 集計（キー名揺れに耐える）
          $counts = @{}
          foreach ($it in $issues) {
            $sev = $null
            foreach ($k in @('severityCode','severity_code','severity','sev','severity_level')) {
              if ($it.PSObject.Properties.Name -contains $k -and $it.$k -ne $null -and "$($it.$k)" -ne '') { $sev = "$($it.$k)"; break }
            }
            if ([string]::IsNullOrWhiteSpace($sev)) { $sev = 'unknown' }
            if (!$counts.ContainsKey($sev)) { $counts[$sev] = 0 }
            $counts[$sev]++
          }

          $buildNum = 0
          [int]::TryParse($env:BUILD_NUMBER, [ref]$buildNum) | Out-Null

          $currentObj = [ordered]@{
            buildNumber = $buildNum
            total       = ($issues | Measure-Object).Count
            counts      = $counts
          }
          ($currentObj | ConvertTo-Json -Depth 10) | Set-Content -Path $currentPath -Encoding UTF8
          Write-Host "[INFO] Wrote current severity summary: $currentPath"

          # 推移をジョブ配下に蓄積（Windows前提）
          $historyFile = $null
          if ($env:JENKINS_HOME -and $env:JOB_NAME) {
            $safeJob = $env:JOB_NAME -replace '/', '\\'
            $jobDir  = Join-Path $env:JENKINS_HOME ("jobs\\" + $safeJob)
            if (!(Test-Path $jobDir)) { New-Item -ItemType Directory -Force -Path $jobDir | Out-Null }
            $historyFile = Join-Path $jobDir 'kw_severity_history.json'
          }

          $history = @()
          if ($historyFile -and (Test-Path $historyFile)) {
            try {
              $hraw = Get-Content -Path $historyFile -Raw
              if (![string]::IsNullOrWhiteSpace($hraw)) { $history = $hraw | ConvertFrom-Json }
            } catch { $history = @() }
          }

          # 同一buildNumberがあれば置換、なければ追加
          $newEntry = [ordered]@{
            buildNumber = $buildNum
            total       = $currentObj.total
            counts      = $counts
          }

          $replaced = $false
          for ($i=0; $i -lt $history.Count; $i++) {
            if ($history[$i].buildNumber -eq $buildNum) {
              $history[$i] = $newEntry
              $replaced = $true
              break
            }
          }
          if (-not $replaced) { $history += $newEntry }

          # buildNumber順にソート・直近100件に制限
          $history = $history | Sort-Object buildNumber
          if ($history.Count -gt 100) { $history = $history | Select-Object -Last 100 }

          if ($historyFile) {
            ($history | ConvertTo-Json -Depth 10) | Set-Content -Path $historyFile -Encoding UTF8
            Write-Host "[INFO] Updated history: $historyFile"
          } else {
            Write-Host "[WARN] JENKINS_HOME / JOB_NAME not set. Trend will be only current build."
          }

          # HTML生成（Chart.js CDN）
          $historyJson = '[]'
          if ($historyFile -and (Test-Path $historyFile)) {
            $historyJson = Get-Content -Path $historyFile -Raw
            if ([string]::IsNullOrWhiteSpace($historyJson)) { $historyJson = '[]' }
          }

          $currentJson = Get-Content -Path $currentPath -Raw
          if ([string]::IsNullOrWhiteSpace($currentJson)) { $currentJson = '{}' }

          $html = @"
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Klocwork Dashboard</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Noto Sans JP", Arial, sans-serif; margin: 18px; }
    .grid { display: grid; grid-template-columns: 1fr 2fr; gap: 18px; }
    .card { border: 1px solid #ddd; border-radius: 10px; padding: 14px; }
    h2 { margin: 0 0 10px 0; font-size: 18px; }
    canvas { width: 100% !important; height: 360px !important; }
    .note { color: #666; font-size: 12px; margin-top: 8px; }
  </style>
</head>
<body>
  <h1 style="margin:0 0 14px 0; font-size:22px;">Klocwork Dashboard (severityCode)</h1>

  <div class="grid">
    <div class="card">
      <h2>Overview (Pie) - Current build</h2>
      <canvas id="pie"></canvas>
      <div class="note">Data source: kw_issues.json (severityCode)</div>
    </div>

    <div class="card">
      <h2>History (Bar) - Builds trend</h2>
      <canvas id="bar"></canvas>
      <div class="note">Trend is stored in: \$JENKINS_HOME\\jobs\\\$JOB_NAME\\kw_severity_history.json (last 100 builds)</div>
    </div>
  </div>

  <script>
    window.__KW_CURRENT__ = $currentJson;
    window.__KW_HISTORY__ = $historyJson;
  </script>

  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <script>
    // ---- Pie (current) ----
    const cur = window.__KW_CURRENT__ || {};
    const curCounts = (cur.counts || {});
    const pieLabels = Object.keys(curCounts).sort();
    const pieData   = pieLabels.map(k => curCounts[k] || 0);

    new Chart(document.getElementById('pie'), {
      type: 'doughnut',
      data: { labels: pieLabels, datasets: [{ data: pieData }] },
      options: {
        plugins: {
          legend: { position: 'bottom' },
          title: { display: true, text: `Build #${cur.buildNumber ?? ''} (Total: ${cur.total ?? 0})` }
        }
      }
    });

    // ---- Bar (history) ----
    const hist = Array.isArray(window.__KW_HISTORY__) ? window.__KW_HISTORY__ : [];
    const allKeys = new Set();
    hist.forEach(h => Object.keys(h.counts || {}).forEach(k => allKeys.add(k)));
    const keys = Array.from(allKeys).sort();

    const labels = hist.map(h => `#${h.buildNumber}`);
    const datasets = keys.map(k => ({
      label: k,
      data: hist.map(h => (h.counts && h.counts[k]) ? h.counts[k] : 0),
      stack: 'sev'
    }));

    new Chart(document.getElementById('bar'), {
      type: 'bar',
      data: { labels, datasets },
      options: {
        responsive: true,
        scales: {
          x: { stacked: true },
          y: { stacked: true, beginAtZero: true }
        },
        plugins: {
          legend: { position: 'bottom' },
          title: { display: true, text: 'severityCode trend (stacked bar)' }
        }
      }
    });
  </script>
</body>
</html>
"@

          $html | Set-Content -Path $htmlPath -Encoding UTF8
          Write-Host "[INFO] Wrote dashboard HTML: $htmlPath"
        '''
      }
    }

    // Warnings NG（従来どおり）
    stage('Show issues as table in Jenkins (Warnings NG)') {
      steps {
        recordIssues(
          enabledForFailure: true,
          tools: [sarif(pattern: "${env.KW_ISSUES_SARIF}")]
        )
      }
    }

    // ★追加：HTMLレポート公開（要: HTML Publisher plugin）
    stage('Publish Klocwork Dashboard') {
      steps {
        publishHTML(target: [
          reportDir: "${env.WORKSPACE}",
          reportFiles: "${env.KW_DASHBOARD_HTML}",
          reportName: "Klocwork Dashboard (severityCode)",
          keepAll: true,
          alwaysLinkToLastBuild: true,
          allowMissing: true
        ])
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_SEV_CURRENT_JSON}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_DASHBOARD_HTML}", allowEmptyArchive: true
    }
  }
}
