pipeline {
    agent {
        docker { image 'fare-docker-reg.dock.studios:5000/docker-images/cpp-static-code-analysis:latest' }
    }
    stages {
        stage('build') {
            steps {
                sh 'cmake .'
                sh 'make all'
            }
        }
    }
}
