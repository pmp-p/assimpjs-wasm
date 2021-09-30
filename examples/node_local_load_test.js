let fs = require ('fs');
const assimpjs = require ('../dist/assimpjs.js')();

assimpjs.then ((ajs) => {
    // create new file list object
    let fileList = new ajs.FileList ();
    
    // add model files
    fileList.AddFile (
        'cube_with_materials.obj',
        fs.readFileSync ('testfiles/cube_with_materials.obj')
    );
    fileList.AddFile (
        'cube_with_materials.mtl',
        fs.readFileSync ('testfiles/cube_with_materials.mtl')
    );
    
    // import model
    let result = ajs.ImportFileList (fileList);
    
    // parse the result json
    let resultJson = JSON.parse (result);
    
    console.log (resultJson);
});
