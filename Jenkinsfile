pipeline {
  agent any
  options { skipDefaultCheckout(true) }

  environment {
    // === ビルド設定 ===
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    // === Klocwork Jenkins プラグイン設定名（Jenkinsの設定に合わせて変更）===
    // Jenkins > Manage Jenkins > Global Tool Configuration > Klocwork の Installations 名
    KW_INSTALL_CONFIG = 'Klocwork 2025.4'

    // Jenkins > Manage Jenkins > Configure System > Klocwork Server Config の設定名
    KW_SERVER_CONFIG  = 'Validateサーバー'

    // Validate 側のプロジェクト名（serverProject / kwciagent set で使用）
    KW_PROJECT = 'jenkins_demo'

    // ltoken（BuildWrapperが自動検出しているなら無くても可）
    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
        bat '''@echo on
git rev-parse --is-inside-work-tree
git fetch --tags --prune --progress
git fetch --unshallow || exit /b 0
'''
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {
        klocworkWrapper(
          installConfig : "${env.KW_INSTALL_CONFIG}",
          serverConfig  : "${env.KW_SERVER_CONFIG}",
          serverProject : "${env.KW_PROJECT}",
          ltoken        : "${env.KW_LTOKEN}"
        ) {
          bat '''@echo on
setlocal

REM ===== clean =====
if exist "%WORKSPACE%\\kwinject.out" del /f /q "%WORKSPACE%\\kwinject.out"
if exist "%WORKSPACE%\\kwtables"   rmdir /s /q "%WORKSPACE%\\kwtables"
if exist "%WORKSPACE%\\diff_file_list_raw.txt" del /f /q "%WORKSPACE%\\diff_file_list_raw.txt"
if exist "%WORKSPACE%\\diff_file_list.txt"     del /f /q "%WORKSPACE%\\diff_file_list.txt"

REM ===== diff list (raw) =====
git rev-parse --verify HEAD 1>nul 2>nul || (echo [ERROR] HEAD not found & exit /b 1)

REM HEAD~1 が無いケースは空にする
git rev-parse --verify HEAD~1 1>nul 2>nul && (
  git diff --name-only HEAD~1 HEAD 1>"%WORKSPACE%\\diff_file_list_raw.txt"
) || (
  type nul 1>"%WORKSPACE%\\diff_file_list_raw.txt"
)

echo ===== diff_file_list_raw.txt =====
type "%WORKSPACE%\\diff_file_list_raw.txt"
echo ================================

REM ===== diff list (Klocwork用) =====
REM - 変更ファイルのうち、.c/.cc/.cpp/.cxx のみ対象
REM - buildspec(kwinject.out) が ..\\revisions\\... を参照する前提に合わせて ..\\ を付与
setlocal EnableDelayedExpansion
type nul 1>"%WORKSPACE%\\diff_file_list.txt"

for /F "usebackq delims=" %%F in ("%WORKSPACE%\\diff_file_list_raw.txt") do (
  set "p=%%F"
  if not "!p!"=="" (
    set "ext=%%~xF"
    if /I "!ext!"==".c"   (set "p=!p:/=\\!" & echo ..\\!p!>>"%WORKSPACE%\\diff_file_list.txt")
    if /I "!ext!"==".cc"  (set "p=!p:/=\\!" & echo ..\\!p!>>"%WORKSPACE%\\diff_file_list.txt")
    if /I "!ext!"==".cpp" (set "p=!p:/=\\!" & echo ..\\!p!>>"%WORKSPACE%\\diff_file_list.txt")
    if /I "!ext!"==".cxx" (set "p=!p:/=\\!" & echo ..\\!p!>>"%WORKSPACE%\\diff_file_list.txt")
  )
)

endlocal

echo ===== diff_file_list.txt =====
type "%WORKSPACE%\\diff_file_list.txt"
echo ============================

REM ===== build spec generation =====
kwinject --version
kwinject --output "%WORKSPACE%\\kwinject.out" "%MSBUILD%" "%SLN%" /t:Rebuild
if errorlevel 1 exit /b 1
if not exist "%WORKSPACE%\\kwinject.out" exit /b 1

REM ===== IMPORTANT: cwd 対策 =====
REM kwinject.out 内が ..\\revisions\\... を含むので、
REM %WORKSPACE%\\_kwcwd に cd してから kwciagent run すると
REM ..\\revisions\\... => %WORKSPACE%\\revisions\\... に解決される
if not exist "%WORKSPACE%\\_kwcwd" mkdir "%WORKSPACE%\\_kwcwd"

REM ===== kwciagent set/run =====
kwciagent --version

kwciagent set --project-dir "%WORKSPACE%\\.kwlp" klocwork.host=192.168.137.1 klocwork.port=2540 klocwork.project=%KW_PROJECT%
if errorlevel 1 exit /b 1

pushd "%WORKSPACE%\\_kwcwd"
kwciagent run ^
  --project-dir "%WORKSPACE%\\.kwlp" ^
  --license-host 192.168.137.1 ^
  --license-port 27000 ^
  -Y -L ^
  --build-spec "%WORKSPACE%\\kwinject.out" ^
  @"%WORKSPACE%\\diff_file_list.txt"
set rc=%ERRORLEVEL%
popd

exit /b %rc%
'''
        }
      }
    }
  }

  post {
    always {
      archiveArtifacts artifacts: 'kwinject.out,diff_file_list_raw.txt,diff_file_list.txt,kwtables/**', allowEmptyArchive: true
    }
  }
}
