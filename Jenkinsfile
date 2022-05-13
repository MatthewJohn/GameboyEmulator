pipeline {
    agent {
        docker { image 'registry.gitlab.dockstudios.co.uk/pub/docker-images/cpp-static-code-analysis:latest' }
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
