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

    // 追加：差分指摘の出力（warnings-ng に渡す）
    KW_DIFF_ISSUES_TSV   = 'diff_issues.tsv'
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
          // 優先順：前回成功 → 前回 → HEAD~1 → (取れなければ空)
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
                // 差分解析
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
                // 初回など previous commit が取れない時はフル解析
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

    stage('Export diff issues to SARIF (for Warnings NG)') {
      when {
        expression { return env.KW_PREV_COMMIT?.trim() }
      }
      steps {
        // 1) 差分ファイルに限定して “指摘一覧” を取得（HTMLは無理なのでscriptable/tsvで取る）
        bat """
          @echo off
          echo [INFO] Export Klocwork diff issues to %KW_DIFF_ISSUES_TSV%
          rem scriptable はタブ区切りで扱いやすい（環境により出力形式が多少違っても吸収しやすい）
          kwciagent list --project-dir "%WORKSPACE%\\.kwlp" --license-host 192.168.137.1 --license-port 27000 -F scriptable @%DIFF_FILE_LIST% > "%KW_DIFF_ISSUES_TSV%"
          type "%KW_DIFF_ISSUES_TSV%"
        """

        // 2) TSV -> SARIF（Warnings NG の sarif 解析で “表” にできる）
        powershell '''
          $tsv = Join-Path $env:WORKSPACE $env:KW_DIFF_ISSUES_TSV
          $sarifPath = Join-Path $env:WORKSPACE $env:KW_DIFF_ISSUES_SARIF

          if (!(Test-Path $tsv)) {
            Write-Host "[WARN] TSV not found: $tsv"
            # 空のSARIFでも作っておく（recordIssuesが落ちないように）
            $empty = @{
              '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
              version='2.1.0'
              runs=@(@{ tool=@{ driver=@{ name='Klocwork (diff)'; rules=@() } }; results=@() })
            } | ConvertTo-Json -Depth 20
            Set-Content -Path $sarifPath -Value $empty -Encoding UTF8
            exit 0
          }

          $lines = Get-Content $tsv
          $results = @()

          foreach ($line in $lines) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }

            # 期待：tab区切り（例）
            # id <TAB> file <TAB> function <TAB> code <TAB> message <TAB> ... <TAB> severity ...
            $cols = $line -split "`t"
            if ($cols.Count -lt 5) { continue }

            $file = $cols[1]
            $ruleId = $cols[3]
            $msg = $cols[4]

            # line情報が無いケースが多いので 1 に寄せる（場所表示だけでも表は出せる）
            $results += @{
              ruleId = $ruleId
              message = @{ text = $msg }
              locations = @(@{
                physicalLocation = @{
                  artifactLocation = @{ uri = $file }
                  region = @{ startLine = 1 }
                }
              })
            }
          }

          $sarif = @{
            '$schema'='https://schemastore.azurewebsites.net/schemas/json/sarif-2.1.0.json'
            version='2.1.0'
            runs=@(@{
              tool=@{ driver=@{ name='Klocwork (diff)'; informationUri=''; rules=@() } }
              results=$results
            })
          } | ConvertTo-Json -Depth 20

          Set-Content -Path $sarifPath -Value $sarif -Encoding UTF8
          Write-Host "[INFO] Wrote SARIF: $sarifPath"
        '''
      }
    }

    stage('Show issues as table in Jenkins (Warnings NG)') {
      when {
        expression { return env.KW_PREV_COMMIT?.trim() }
      }
      steps {
        // Warnings Next Generation で “表” 表示
        // ※ plugin が入っていれば、ビルド画面に Warnings のリンクが出ます
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
      archiveArtifacts artifacts: "${env.KW_DIFF_ISSUES_TSV}", allowEmptyArchive: true
      archiveArtifacts artifacts: "${env.KW_DIFF_ISSUES_SARIF}", allowEmptyArchive: true
    }
  }
}
