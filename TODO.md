1. Factorise to models, utils etc (DONE)
2. Fix issue with cluster causing memory problems 
3. create structure to save and load vocab features
4. create structure to store task tree when tree building
 what would the index look like?
 {
     "partitions": 8,
     "centroids": [0, 1, 2, 3];
     "membership": {
         0: [{recursive}]
         1: []
         2: []
         3: []
     },
     features: [
         x, y, z, a
     ]
 }