pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // パスは「クォート無し」で持って、使うときに必ず " " で囲う
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // ここは Jenkins の「Klocwork Server Config」の名前に合わせる
    // 例：UIで作った設定名が "Validateサーバー" ならこれでOK
    KW_SERVER_CONFIG = 'Validateサーバー'
    KW_PROJECT       = 'jenkins_demo'

    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {
    stage('Checkout') {
      steps {
        // まず通常 checkout
        checkout scm

        // shallow clone 等で HEAD~1 が無い問題を潰す（失敗しても続行）
        bat '''
          @echo on
          git rev-parse --is-inside-work-tree
          git fetch --tags --prune --progress
          git fetch --unshallow || exit /b 0
        '''
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {
        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_PROJECT}"
        ) {

          // 毎回クリーン
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'
          bat 'if exist diff_file_list.txt del /f /q diff_file_list.txt'

          // 差分ファイル一覧を作成（重要：ちゃんと > diff_file_list.txt する）
          // HEAD~1 が無い場合は空ファイルにする
          bat '''
            @echo on
            git rev-parse --verify HEAD >nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)
            git rev-parse --verify HEAD~1 >nul 2>nul && (
              git diff --name-only HEAD~1 HEAD > diff_file_list.txt
            ) || (
              type nul > diff_file_list.txt
            )
          '''

          // diff_file_list.txt の中身をコンソールに表示（確認用）
          bat '''
            @echo on
            echo ===== diff_file_list.txt =====
            if exist diff_file_list.txt type diff_file_list.txt
            for %%A in (diff_file_list.txt) do @echo [INFO] diff_file_list.txt bytes=%%~zA
            echo =============================
          '''

          // 1) kwinject（"C:\Program" is not executable 対策：必ず二重引用符）
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"${env.MSBUILD}\" \"${env.SLN}\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード（空ファイル事故防止）
          bat 'if not exist kwinject.out exit /b 1'
          bat 'for %%A in (kwinject.out) do if %%~zA==0 exit /b 1'

          // 2) 差分解析（Diff Analysis）
          // ※パラメータ名はあなたが使っていた "differentialAnalysisConfig" に合わせてます
          klocworkIncremental([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList: 'diff_file_list.txt',
              diffType: 'git',
              // ここが空だとプラグイン側で解釈が揺れることがあるので明示推奨
              gitPreviousCommit: 'HEAD~1'
            ],
            // Diff Analysis を動かしたいなら通常 true の方が意図に合います
            incrementalAnalysis: true,
            projectDir: '',
            reportFile: ''
          ])

          // 3) Jenkinsへ指摘を同期（Dashboard表示を安定させる狙い）
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

          // 必要ならゲートや同期もここに追加
          // klocworkQualityGateway([...])
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list.txt,kwtables/**', onlyIfSuccessful: false
    }
  }
}
