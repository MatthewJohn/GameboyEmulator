pipeline {
    agent any
    stages {
        stage('build') {
            agent {
                docker { image 'fare-docker-reg.dock.studios:5000/docker-images/cpp-static-code-analysis:latest' }
            }

            steps {
                sh 'rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake'
                sh 'cmake .'
                sh 'make clean all'
                stash includes: 'GameboyEmulator', name: 'app'
            }
        }
        stage('System tests') {
            matrix {
               agent {
                    docker { image 'fare-docker-reg.dock.studios:5000/docker-images/screenshot-tool:latest' }
               }
 
               axis {
                    name 'TEST_NAME'
                    values '01-special', '02-interrupts', '03-op_sp,hl', '04-op_r,imm', '05-op_rp', '06-ld_r,r', '07-jr,jp,call,ret,rst', '08-misc-instrs', '09-op_r,r', '10-bit_ops', '11-op_a,-hl'
                }
                stages {
                    stage('Perform test') {

                        steps {
                          unstash 'app'
                          sh "bash ./tests/system/${TEST_NAME}.sh"
                        }
                        post {
                            always {
                                archiveArtifacts artifacts: 'output.bmp,comparison.jpg', fingerprint: false
                            }
                        }
                    }
                }
            }
        }
    }
}
