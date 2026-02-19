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
    MAKE_ARGS    = ''

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    KW_BUILD_SPEC = 'kwinject.out'

    // Warnings NG に渡す（今回は“全体解析”の指摘を表で見せる）
    KW_ISSUES_TSV   = 'kw_issues.tsv'
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

            // ★ここを「差分」ではなく「全体解析」に固定
            script {
              klocworkIncremental([
                additionalOpts     : '',
                buildSpec          : "${env.KW_BUILD_SPEC}",
                cleanupProject     : false,
                incrementalAnalysis: false,   // ← FULL
                projectDir         : '',
                reportFile         : ''
              ])
            }
          }
        }
      }
    }

    stage('Export issues to SARIF (for Warnings NG)') {
      steps {
        // 1) 指摘一覧をTSVで吐く（scriptable=タブ区切りで扱いやすい）
        bat """
          @echo off
          echo [INFO] Export Klocwork issues to %KW_ISSUES_TSV%
          rem 全体の指摘を出す（ファイルリスト指定なし）
          kwciagent list --project-dir "%WORKSPACE%\\.kwlp" --license-host 192.168.137.1 --license-port 27000 -F scriptable > "%KW_ISSUES_TSV%"
          echo [INFO] TSV size:
          for %%A in ("%KW_ISSUES_TSV%") do echo %%~zA bytes
        """

        // 2) TSV -> SARIF（JenkinsのWarnings NGで“表”表示）
        //    ※ PowerShell 5.x 前提で書く（?. を使わない）
        powershell '''
          $tsv = Join-Path $env:WORKSPACE $env:KW_ISSUES_TSV
          $sarifPath = Join-Path $env:WORKSPACE $env:KW_ISSUES_SARIF

          function Write-EmptySarif($path) {
            $emptyObj = @{
              '$schema' = 'https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
              version   = '2.1.0'
              runs      = @(@{
                tool    = @{ driver = @{ name = 'Klocwork'; rules = @() } }
                results = @()
              })
            }
            $json = $emptyObj | ConvertTo-Json -Depth 20
            Set-Content -Path $path -Value $json -Encoding UTF8
          }

          if (!(Test-Path $tsv)) {
            Write-Host "[WARN] TSV not found: $tsv"
            Write-EmptySarif $sarifPath
            exit 0
          }

          $lines = Get-Content -Path $tsv
          $results = New-Object System.Collections.Generic.List[Object]

          foreach ($line in $lines) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }

            # 想定: tab区切り（例）
            # id<TAB>file<TAB>function<TAB>code<TAB>message<TAB>...<TAB>severity...
            $cols = $line -split "`t"
            if ($cols.Count -lt 5) { continue }

            $file  = $cols[1]
            $rule  = $cols[3]
            $msg   = $cols[4]

            if ([string]::IsNullOrWhiteSpace($rule)) { $rule = 'KLOCWORK' }
            if ([string]::IsNullOrWhiteSpace($file)) { $file = 'unknown' }
            if ([string]::IsNullOrWhiteSpace($msg))  { $msg  = '' }

            $results.Add(@{
              ruleId   = $rule
              message  = @{ text = $msg }
              locations = @(@{
                physicalLocation = @{
                  artifactLocation = @{ uri = $file }
                  region = @{ startLine = 1 }   # 行番号が無い前提 → 1固定
                }
              })
            })
          }

          $sarifObj = @{
            '$schema' = 'https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version   = '2.1.0'
            runs      = @(@{
              tool    = @{ driver = @{ name = 'Klocwork'; rules = @() } }
              results = $results
            })
          }

          $sarifJson = $sarifObj | ConvertTo-Json -Depth 20
          Set-Content -Path $sarifPath -Value $sarifJson -Encoding UTF8
          Write-Host "[INFO] Wrote SARIF: $sarifPath"
          Write-Host ("[INFO] SARIF results: {0}" -f $results.Count)
        '''
      }
    }

    stage('Show issues as table in Jenkins (Warnings NG)') {
      steps {
        // Warnings Next Generation が入っていれば、
        // ビルド画面に「Warnings」(または Issues) のリンクが出て、表で見られます。
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
      archiveArtifacts artifacts: "${env.KW_ISSUES_TSV}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
