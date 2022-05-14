pipeline {
    agent {
        docker { image 'fare-docker-reg.dock.studios:5000/docker-images/cpp-static-code-analysis:latest' }
    }
    stages {
        stage('build') {
            steps {
                sh 'cmake .'
                sh 'make all'
                stash includes: './GameboyEmultor', name: 'app'
            }
        }
        stage('Test 01-special') {
            agent {
                docker { image 'fare-docker-reg.dock.studios:5000/docker-images/screenshot-tool:latest' }
            }
            steps {
              unstash 'app'
              sh 'bash ./tests/system/01-special.sh'
            }
        }
    }
}
