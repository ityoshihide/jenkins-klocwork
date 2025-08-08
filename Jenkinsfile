pipeline {
    agent any

    environment {
        KW_PATH = "/opt/klocwork/kwbuildtools/bin"
    }

    stages {
        stage('Checkout') {
            steps {
                echo '[INFO] Checking out the repository...'
                checkout scm
            }
        }

        stage('Run kwinject') {
            steps {
                echo '[INFO] Running kwinject with full path..'
                sh '''
                    ${KW_PATH}/kwinject make
                '''
            }
        }

        stage('Verify kwinject.out') {
            steps {
                echo '[INFO] Verifying kwinject.out..'
                sh '''
                    if [ -f kwinject.out ]; then
                        echo "[INFO] kwinject.out generated successfully."
                    else
                        echo "[ERROR] kwinject.out not found!"
                        exit 1
                    fi
                '''
            }
        }

        stage('Run kwbuildproject') {
            steps {
                echo '[INFO] Running kwbuildproject...'
                sh '''
                    ${KW_PATH}/kwbuildproject --url http://192.50.110.196:2520/jenkins -o table kwinject.out -f
                '''
            }
        }

        stage('Run kwadmin') {
            steps {
                echo '[INFO] Loading project into Klocwork server using kwadmin...'
                sh '''
                    ${KW_PATH}/kwadmin --url http://192.50.110.196:2520 load jenkins table
                '''
            }
        }
    }
}
