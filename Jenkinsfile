pipeline {
    agent any

    environment {
        KW_PATH = "/home/ec2-user/kwbuildtools/kwbuildtools/bin"
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
                echo '[INFO] Running kwinject with full path...'
                sh '''
                    ${KW_PATH}/kwinject make
                '''
            }
        }

        stage('Verify kwinject.out') {
            steps {
                echo '[INFO] Verifying kwinject.out...'
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
    }
}


