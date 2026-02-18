pipeline {
  agent any

  options {
    skipDefaultCheckout(true)
    disableConcurrentBuilds()
    timestamps()
  }

  environment {
    // Jenkins 管理画面（Klocwork の Server Configurations）で登録した「Name」と一致させる
    // すでにあなたの環境ではURL(192.168.137.1:2540)が設定されて動いている前提
    KW_SERVER_CONFIG = 'Validateサーバー'

    // Klocwork サーバープロジェクト名
    KW_PROJECT = 'jenkins_demo'

    // ltoken（Jenkins 実行ユーザーから参照できるパス）
    KW_LTOKEN  = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'

    // VS / MSBuild
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'
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
          // 例: jenkins-klocwork-pipeline-23
          env.KW_BUILD_NAME = "jenkins-${env.JOB_NAME}-${env.BUILD_NUMBER}"
        }

        klocworkWrapper(
          installConfig: '-- なし --',
          ltoken: "${env.KW_LTOKEN}",
          serverConfig: "${env.KW_SERVER_CONFIG}",
          serverProject: "${env.KW_PROJECT}"
        ) {

          // --- Clean ---
          bat 'if exist kwinject.out del /f /q kwinject.out'
          bat 'if exist kwtables rmdir /s /q kwtables'
          bat 'if exist diff_file_list.txt del /f /q diff_file_list.txt'

          // ★ PDB (vc143.pdb) 競合/破損対策：Debug 出力を消す（失敗してもパイプラインを落とさない）
          bat '''
@echo off
setlocal
if exist "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\Debug" (
  rmdir /s /q "C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\Debug" >nul 2>nul
)
exit /b 0
'''

          // --- Diff file list (Git) ---
          // push直後や shallow clone で HEAD~1 が無い場合があるので、失敗しても落とさない
          bat '''
@echo off
setlocal

git rev-parse --verify HEAD>nul 2>nul || (
  echo [WARN] HEAD is not available.
  type nul > diff_file_list.txt
  exit /b 0
)

git rev-parse --verify HEAD~1>nul 2>nul || (
  echo [WARN] HEAD~1 is not available (shallow clone or first commit). diff_file_list will be empty.
  type nul > diff_file_list.txt
  exit /b 0
)

git diff --name-only HEAD~1 HEAD > diff_file_list.txt

for %%A in (diff_file_list.txt) do if %%~zA==0 (
  echo [WARN] diff_file_list.txt is empty. Diff Analysis may have nothing to show.
)

exit /b 0
'''

          // --- 1) BuildSpec generation (kwinject) ---
          klocworkBuildSpecGeneration([
            additionalOpts: '',
            buildCommand: "\"%MSBUILD%\" \"%SLN%\" /t:Rebuild",
            ignoreErrors: false,
            output: 'kwinject.out',
            tool: 'kwinject',
            workDir: ''
          ])

          // Guard: kwinject.out must exist and be non-empty
          bat 'if not exist kwinject.out exit /b 1'
          bat 'for %%A in (kwinject.out) do if %%~zA==0 exit /b 1'

          // --- 2) Diff / Incremental (Plugin) ---
          // ここが「Jenkins UI に Diff Analysis Issues を出す」ための本体
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

          // 任意：ビルド説明に情報を載せる
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
