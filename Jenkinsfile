pipeline {
  agent any

  options {
    skipDefaultCheckout(true)
    disableConcurrentBuilds()
    timestamps()
  }

  environment {
    KW_SERVER_CONFIG = 'Validateサーバー'  // Jenkins側の Klocwork Server Configurations の Name
    KW_PROJECT       = 'jenkins_demo'
    KW_LTOKEN        = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'

    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // PDBの掃除対象（固定パスでビルドしているため）
    VS_DEBUG_DIR = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\Debug'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {
        script {
          env.KW_BUILD_NAME = "jenkins-${env.JOB_NAME}-${env.BUILD_NUMBER}"
        }

        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_PROJECT}"
        ) {

          // クリーン（全部 1行 cmd /c で統一）
          bat 'cmd /c if exist kwinject.out del /f /q kwinject.out'
          bat 'cmd /c if exist kwtables rmdir /s /q kwtables'
          bat 'cmd /c if exist diff_file_list.txt del /f /q diff_file_list.txt'

          // ★ ここが以前 255 で落ちていた箇所：失敗しても落とさない書き方に固定
          bat 'cmd /c (if exist "%VS_DEBUG_DIR%" rmdir /s /q "%VS_DEBUG_DIR%") >nul 2>nul & exit /b 0'

          // diff_file_list を作る（HEAD~1 が無い/差分ゼロでも落とさない）
          bat 'cmd /c (git rev-parse --verify HEAD>nul 2>nul && git rev-parse --verify HEAD~1>nul 2>nul && git diff --name-only HEAD~1 HEAD > diff_file_list.txt) >nul 2>nul & (if not exist diff_file_list.txt type nul > diff_file_list.txt) & exit /b 0'
          bat 'cmd /c for %%A in (diff_file_list.txt) do @if %%~zA==0 echo [WARN] diff_file_list.txt is empty. Diff Analysis may show nothing.'

          // 1) BuildSpec (kwinject)
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"%MSBUILD%\" \"%SLN%\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // ガード
          bat 'cmd /c if not exist kwinject.out exit /b 1'
          bat 'cmd /c for %%A in (kwinject.out) do @if %%~zA==0 exit /b 1'

          // 2) Diff Analysis
          klocworkIncremental([
            additionalOpts: '',
            buildSpec: 'kwinject.out',
            cleanupProject: false,
            differentialAnalysisConfig: [
              diffFileList: 'diff_file_list.txt',
              diffType: 'git',
              gitPreviousCommit: 'HEAD~1'
            ],
            incrementalAnalysis: false,
            projectDir: '',
            reportFile: ''
          ])

          script {
            currentBuild.description = "Klocwork Diff: project=${env.KW_PROJECT} / build=${env.KW_BUILD_NAME}"
          }
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
