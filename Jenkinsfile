pipeline {
  agent any
  triggers { githubPush() }

  options {
    skipDefaultCheckout(true)
    timestamps()
  }

  environment {
    MSYS2_ROOT = 'C:\\msys64'
    MAKE_WORKDIR = '.'
    MAKE_ARGS = ''

    KW_LTOKEN         = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
    KW_SERVER_CONFIG  = 'Validateサーバー'
    KW_SERVER_PROJECT = 'jenkins_demo'

    DIFF_FILE_LIST = 'diff_file_list.txt'
    KW_BUILD_SPEC  = 'kwinject.out'

    // Warnings NG 用
    KW_DIFF_ISSUES_XML   = 'diff_issues.xml'
    KW_DIFF_ISSUES_SARIF = 'diff_issues.sarif'
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
          def prev = env.GIT_PREVIOUS_SUCCESSFUL_COMMIT
          if (!prev?.trim()) prev = env.GIT_PREVIOUS_COMMIT

          if (!prev?.trim()) {
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

    stage('Export diff issues XML -> SARIF (for Warnings)') {
      when { expression { return env.KW_PREV_COMMIT?.trim() } }
      steps {
        // 1) XMLをファイルへ出力（ここが一番確実）
        bat """
          @echo off
          echo [INFO] Export Klocwork diff issues to %KW_DIFF_ISSUES_XML%
          kwciagent list --project-dir "%WORKSPACE%\\.kwlp" --license-host 192.168.137.1 --license-port 27000 -F xml @%DIFF_FILE_LIST% > "%KW_DIFF_ISSUES_XML%"
          echo [INFO] diff issues xml size:
          for %%A in ("%KW_DIFF_ISSUES_XML%") do echo %%~zA bytes
        """

        // 2) XML -> SARIF
        powershell '''
          $xmlPath   = Join-Path $env:WORKSPACE $env:KW_DIFF_ISSUES_XML
          $sarifPath = Join-Path $env:WORKSPACE $env:KW_DIFF_ISSUES_SARIF

          $emptySarif = @{
            '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version='2.1.0'
            runs=@(@{ tool=@{ driver=@{ name='Klocwork (diff)'; rules=@() } }; results=@() })
          } | ConvertTo-Json -Depth 50

          if (!(Test-Path $xmlPath)) {
            Write-Host "[WARN] XML not found: $xmlPath"
            Set-Content -Path $sarifPath -Value $emptySarif -Encoding UTF8
            exit 0
          }

          $size = (Get-Item $xmlPath).Length
          if ($size -eq 0) {
            Write-Host "[WARN] XML is empty"
            Set-Content -Path $sarifPath -Value $emptySarif -Encoding UTF8
            exit 0
          }

          [xml]$x = Get-Content -Path $xmlPath

          # XML構造が環境で違う可能性があるので、"issue" っぽいノードを広めに拾う
          $issueNodes = @()
          $issueNodes += $x.SelectNodes("//*[local-name()='issue']")
          $issueNodes += $x.SelectNodes("//*[local-name()='problem']")
          $issueNodes += $x.SelectNodes("//*[local-name()='defect']")

          $results = @()

          foreach ($n in $issueNodes) {
            # ありがちな要素名を候補として拾う（無ければ空でOK）
            $file = ($n.SelectSingleNode(".//*[local-name()='file']")?.InnerText)
            if (-not $file) { $file = ($n.SelectSingleNode(".//*[local-name()='path']")?.InnerText) }

            $line = ($n.SelectSingleNode(".//*[local-name()='line']")?.InnerText)
            if (-not $line) { $line = 1 }
            [int]$line = $line

            $rule = ($n.SelectSingleNode(".//*[local-name()='code']")?.InnerText)
            if (-not $rule) { $rule = ($n.SelectSingleNode(".//*[local-name()='checker']")?.InnerText) }
            if (-not $rule) { $rule = "Klocwork" }

            $msg  = ($n.SelectSingleNode(".//*[local-name()='message']")?.InnerText)
            if (-not $msg) { $msg = ($n.SelectSingleNode(".//*[local-name()='description']")?.InnerText) }
            if (-not $msg) { $msg = "Klocwork issue" }

            if (-not $file) { $file = "unknown" }

            $results += @{
              ruleId = $rule
              message = @{ text = $msg }
              locations = @(@{
                physicalLocation = @{
                  artifactLocation = @{ uri = $file }
                  region = @{ startLine = $line }
                }
              })
            }
          }

          $sarif = @{
            '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version='2.1.0'
            runs=@(@{
              tool=@{ driver=@{ name='Klocwork (diff)'; rules=@() } }
              results=$results
            })
          } | ConvertTo-Json -Depth 50

          Set-Content -Path $sarifPath -Value $sarif -Encoding UTF8
          Write-Host "[INFO] Wrote SARIF: $sarifPath (results: $($results.Count))"
        '''
      }
    }

    stage('Show issues as table in Jenkins (Warnings)') {
      when { expression { return env.KW_PREV_COMMIT?.trim() } }
      steps {
        recordIssues(
          enabledForFailure: true,
          tools: [sarif(pattern: "${env.KW_DIFF_ISSUES_SARIF}")]
        )
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: "${env.DIFF_FILE_LIST}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_BUILD_SPEC}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_DIFF_ISSUES_XML}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_DIFF_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
