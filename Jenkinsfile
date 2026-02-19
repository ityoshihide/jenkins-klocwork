pipeline {
  agent any

  options {
    skipDefaultCheckout(true)
  }

  environment {
    MSBUILD = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe'
    SLN     = 'C:\\Klocwork\\CommandLine25.4\\samples\\demosthenes\\vs2022\\4.sln'

    KW_SERVER_URL = 'http://192.168.137.1:2540'
    KW_LICENSE_HOST = '192.168.137.1'
    KW_LICENSE_PORT = '27000'
    KW_PROJECT = 'jenkins_demo'

    KW_LTOKEN = 'C:\\Users\\MSY11199\\.klocwork\\ltoken'
  }

  stages {

    stage('Checkout') {
      steps {
        checkout scm
        bat '''
          git rev-parse --is-inside-work-tree
          git fetch --tags --prune --progress
          git fetch --unshallow || exit /b 0
        '''
      }
    }

    stage('Klocwork Diff Analysis') {
      steps {
        klocworkWrapper {

          // 既存ファイル削除
          bat '''
            if exist kwinject.out d
